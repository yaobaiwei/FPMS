#ifndef WORKER_H
#define WORKER_H

//#include <vector>

#include <unistd.h> //debug
#include "../utils/global.h"
#include <string>
#include "../utils/communication.h"
#include "../utils/ydhdfs.h"
#include "Slave.h"
#include "Master.h"
#include "Sender.h"
using namespace std;

//ItemT ==> type of items directly read from the file
//TranT ==> type of transactions after processing
//          Usually means the transaction type after calling mapping()
//          After pruning globally infrequent items
//PattT ==> type of pattern
//ProjTranT ==> type of projected transactions 
template <class RItemT, class PDBT>
class Worker {

public:
	//types
    //Record ==> the initial unpruned data, used in initial pruning
    typedef RItemT ReadInItemT;
    typedef vector<ReadInItemT> Record;
    typedef vector<Record> RecordContainer;
    typedef unordered_map<ReadInItemT, int> ItemTable; //used for initial counting
    //------
    typedef typename PDBT::DeltaType DeltaT;
    //------
    //Transaction ==> the data after pruning and transform
    typedef typename PDBT::TranType Transaction;
    typedef typename Transaction::ItemType TItemT;
    typedef vector<Transaction> TransactionContainer;

    typedef typename PDBT::ProjTranType ProjTranT;
    typedef typename PDBT::PattType PattT;
    typedef SlaveNode<PattT, PDBT> SNode;
    typedef Slave<SNode> SlaveT;

    typedef MasterNode<PattT> MNode;
    typedef Master<MNode> MasterT;

    //fields
    TransactionContainer transactions;
    unordered_map<ReadInItemT, int> itemlist; //only useful in master

    Worker()
    {
    	init_workers();
    }

    ~Worker()
    {
    	MPI_Finalize();
    }

    //user-defined graphLoader ==============================
	virtual void to_record(char* line, RecordContainer & records) = 0; //user specifies: line -> tgt

	void load_graph(const char* inpath, RecordContainer & records)
	{
		hdfsFS fs = getHdfsFS();
		hdfsFile in = getRHandle(inpath, fs);
		LineReader reader(fs, in);
		//vector<ItemT> tgt, empty;
		while(true){
			reader.readLine();
			if (!reader.eof())
			{
				to_record(reader.getLine(), records);
				//records.push_back(empty);
				//records.back().swap(tgt);
			}
			else break;
		}
		hdfsCloseFile(fs, in);
		hdfsDisconnect(fs);
		//cout<<"Worker "<<_my_rank<<": \""<<inpath<<"\" loaded"<<endl;//DEBUG !!!!!!!!!!
	}
	//=======================================================

    //construct alphabet of frequent items
    void count_items(RecordContainer & records, ItemTable & alphabet){
    	int record_num = records.size();
    	typename unordered_set<ReadInItemT>::iterator it;
    	typename ItemTable::iterator mit;
        for(int i=0; i<record_num; i++)
        {
        	unordered_set<ReadInItemT> unique_items;
        	//remove redundant items in a record
        	Record & cur = records[i];
        	int len = cur.size();
            for(int j=0; j<len; j++) unique_items.insert(cur[j]);
            //update alphabet table
            for(it = unique_items.begin(); it != unique_items.end(); it++){
                ReadInItemT item = *it;
                mit = alphabet.find(item);
                if(mit == alphabet.end()) alphabet[item] = 1;
                else mit->second++;
            }
        }
    }

    //master aggregate all alphabet tables
    void alphabet_agg(ItemTable & alphabet){
    	int count = 0;
    	typename ItemTable::iterator mit;
        while(count < _num_miners){
        	ItemTable slave_table;
            block_recvData<ItemTable>(slave_table);
        	//update master's alphabet with slave_table
        	typename ItemTable::iterator it;
        	for(it = slave_table.begin(); it != slave_table.end(); it++){
        		mit = alphabet.find(it->first);
				if(mit == alphabet.end()) alphabet[it->first] = it->second;
				else mit->second += it->second;
            }
        	//------
            count ++;
        }
    }

    //run by master: aggregated alphabet -> freq-sorted frequent items

    /*
    void get_itemlist(ItemTable & alphabet) //itemlist = freq-items sorted in non-increasing freq
    {
    	typename ItemTable::iterator it;
    	vector<icpair> vec;
    	icpair tmp;
    	for(it = alphabet.begin(); it != alphabet.end(); it++){
    		tmp.item = it->first;
    		tmp.count = it->second;
    		vec.push_back(tmp);
    	}
    	sort(vec.begin(), vec.end());
    	//alphabet => itemlist
    	for(int i=0; i<vec.size(); i++)
    	{
    		if(vec[i].count >= req_count) itemlist.insert(vec[i].item);
    		else break; //terminate earlier
    	}
    }
    */

    void get_itemlist(ItemTable & alphabet) //itemlist = freq-items sorted in non-increasing freq
    {
        typename ItemTable::iterator it;
        for(it = alphabet.begin(); it != alphabet.end(); it++){
            if(it->second >= req_count)
                itemlist[it->first] = it->second;
        }
    }

    //=======================================================

    virtual void mapping(RecordContainer & records, TransactionContainer  & transactions,  unordered_map<ReadInItemT, int> & itemlist)=0;
    

    // run the worker
    void init(const WorkerParams& params)
    {
        //check input path on HDFS
        if (_my_rank == MASTER_RANK) {
            if (dirCheck(params.input_path.c_str()) == -1) worker_abort();
        }
        init_timers();
        freq_threshold = params.threshold; //set required freq-threshold

        //* load transactions
        //###########################################
        //note: if there are n files (or n mining slaves), should specify (n+1) processes
        //file format:
        //part_1 -> worker 1
        //part_2 -> worker 2
        //...
        //part_n -> worker n
        //###########################################
        ResetTimer(WORKER_TIMER);
        RecordContainer records; //tmp container
        if (_my_rank != MASTER_RANK) {
        	//set tmp as file name "part_{myRank}"
            char tmp[8];
            sprintf(tmp, "%04d", _my_rank-1);
            string fname = params.input_path + "/part_" + tmp;
            //load file lines to records
            load_graph(fname.c_str(), records);
        }
        worker_barrier();
        StopTimer(WORKER_TIMER);
        PrintTimer("Load Time", WORKER_TIMER);

        //=================================================================

        //* count items + alphabet agg + obtain itemlist
        ItemTable alphabet; //tmp container
        if (_my_rank != MASTER_RANK) {
        	//set counts
        	total_count = records.size();
        	//master_sum_LL(total_count); //get total number of records
            slaveGather(total_count);
        	req_count = total_count * freq_threshold;
        	//------------
        	count_items(records, alphabet); //compute alphabet
        	send_data(alphabet, MASTER_RANK); //send alphabet (with local count) to master
        	slaveBcast(itemlist); //sync itemlist: recv
        }
        else
        {
        	//set counts
        	//total_count = master_sum_LL(0); //get total number of records

            /*modify on 8th June, together with slaveGather(total_count) above*/
            _num_trans.resize(_num_workers, 0);
            total_count = 0;
            masterGather(_num_trans);
            for(int i = 1; i<_num_trans.size(); i++)
                total_count += _num_trans[i];
        	req_count = total_count * freq_threshold;
            /******************/

        	cout<<"Total Number of Transactions: "<<total_count<<endl;
        	cout<<"Required Count: "<<req_count<<endl;
        	//------------

        	alphabet_agg(alphabet); //aggregate alphabet tables
        	get_itemlist(alphabet); //alphabet => itemlist
        	masterBcast(itemlist); //sync itemlist: bcast
        }

        //=================================================================

        alphabet.clear(); //itemlist is computed, alphabet can be freed to save space

        //=================================================================

        elements_size = itemlist.size();
        mapping(records, transactions, itemlist);

        //elements_size = mapping.size();

    }//records, alphabet, mapping -> free

    //============================================================================

    // run the worker
	void run(const WorkerParams& params)
	{
		init(params); //read data + set transactions

        //CAPACITY = 20;
        //TIMEOUT = 0.1;
        if(_my_rank != 0)
        {
            SlaveT slave(transactions);
            
            slave.run();

            cout<<"Slave"<<_my_rank<<" completes"<<endl;
            cout<<"===Maximum number of SlaveNodes on Slave"<<_my_rank<<": "<<max_num_SlaveNode<<"/"<<num_SlaveNode<<endl;
            cout<<"===Total batches of message Slave"<<_my_rank<<" "<<send_msg_batch<<" sent."<<endl;
        }
        else
        {
            init_timers();
            cout<<"master pid: "<<getpid()<<endl;

            MasterT master;
            
            master.run();

            cout<<"master side end"<<endl;
            StopTimer(WORKER_TIMER);
            PrintTimer("Mine Time", WORKER_TIMER);
            cout<<">>>Maximum number of MasterNode in memory: "<<max_num_MasterNode<<"/"<<num_MasterNode<<endl;
            cout<<">>>Master totally send "<<total_request<<" requests."<<endl;
            cout<<">>>Total batches of message Master "<<send_msg_batch<<" sent."<<endl;
        }

	}
};

#endif
