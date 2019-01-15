#include "prefixspan/Worker.h"
#include <cstring>

//An example of prefixspan
//ItemID = char

class SeqPatt:public Pattern<ItemID>
{
public:
	SeqPatt()
	{
		next = this->begin();
	}
	
	//UDF
	virtual void grow(const DeltaType & d, Pattern & new_patt)
	{
		for(int i = 0; i < this->size(); i++)
			new_patt.push_back(this->at(i));

		if(d < 0)//assemble to the last itemset
		{	
			new_patt.back() = -new_patt.back();
			new_patt.push_back(d);
		}
		else //append to the prefix, begin of new itemset
		{
			new_patt.push_back(-d);
		}

		new_patt.next = new_patt.begin();
	}

	virtual inline void print()
	{
		cout<<"(";
		for(int i = 0; i < this->size(); i++)
		{
			if(at(i) > 0)
				cout<<at(i)-1<<" ";
			else if( i < this->size() - 1)
				cout<<-at(i)-1<<") (";
			else 
				cout<<-at(i)-1<<")";
		}	
	}

	inline DeltaType next_delta()
	{
		/*
		DeltaType cur = *next;
		next ++;
		if(cur > 0)
			return (-cur);
		else
			return cur;
		*/
		if (next == this->begin())
		{
			DeltaType cur = *next;
			next ++;
			return cur > 0? -cur : cur;
		}
		else
		{
			DeltaType cur = *next;
			next --;
			DeltaType pre = *next;
			next ++;
			next ++;
			if(pre < 0)
				return cur > 0? cur : -cur ;//new itemset, return positive
			else
				return cur > 0? -cur : cur ;//same itemset, return negetive
		}
	}

	DeltaType back()
	{	
		if(size() > 1)
		{
			DeltaType second_last = at(size()-2);
			if(second_last < 0)
				return (DeltaType)-at(size()-1); //last item in new itemset, return positive 
			else
				return at(size()-1); //lasat item in same itemset, return negative
		}
		else
		{
			return at(0); //only one
		}
	}
};

//Transaction type: containing transactions after pruning globally infrequent items;
//					directly consist of database used in frequent pattern mining
class SeqTran: public std::vector<ItemID>
{
public:
	typedef ItemID ItemType;

	SeqTran():vector<ItemID>(){}
};


//Projected transaction type: consist of projected database

class SeqProj: public ProjTran<SeqTran, SeqPatt>
{
public:
	typedef typename SeqTran::iterator Tracker;

	SeqProj(TranType & t, int id):ProjTran(t, id)
	{
		start_pos = t.begin();
	}
	SeqProj(TranType & t, int id, Tracker & pos):ProjTran(t, id)
	{
		start_pos = pos;
	}

	void project(ProjT_map & tmp)
	{
		// assemble to last itemset: < 0
		// append to the prefix: > 0

		if (start_pos == transaction.end()) return;

		//project items that can assemble to the last itemset 
		for(int i = 0; i < partial_starts.size(); i++)
		{
			Tracker & it = partial_starts[i];
			while(*it > 0)//in the same itemset
			{
				DeltaType item = -*it;
				ProjT_map::iterator mit = tmp.find(item);
				if(mit == tmp.end())
				{
					Tracker next = it;
					next ++;
					SeqProj* new_proj = new SeqProj(transaction, tran_id, next);
					new_proj->add_partial_start(next);
					tmp[item] = new_proj;
				}
				else
				{
					Tracker partial_start = it;
					partial_start ++;
					SeqProj* proj = (SeqProj*)mit->second;
					proj->add_partial_start(partial_start);
				}
				it++;
			}
			//last item in the same itemset
			DeltaType item = *it;
			if(tmp.find(item) == tmp.end())
			{
				Tracker next = it;
				next ++;
				tmp[item] = new SeqProj(transaction, tran_id, next);
				//won't add partial starts here
				//because this itemset can't be partial start any more
			}

		}

		//project items that append to the prefix
		Tracker it = start_pos;
		//find the full start
		if(it != transaction.begin())
		{
			it --;
			if(*it > 0)
			{
				for(; it != transaction.end() && *it > 0; it++);
			}
			it ++;
		}

		for(; it != transaction.end(); it++)
		{
			DeltaType item = *it > 0 ? *it : -*it; //set full start greater than 0
			ProjT_map::iterator mit = tmp.find(item);
			if( mit == tmp.end())//not found
			{
				Tracker next = it;
				next ++;
				SeqProj* new_proj = new SeqProj(transaction, tran_id, next);
				if(*it > 0) //not the end of itemset
					new_proj->add_partial_start(next);
				tmp[item] = new_proj;
			}
			else if(*it > 0)
			{
				Tracker next = it;
				next ++;
				SeqProj* proj = (SeqProj*) mit->second;
				proj->add_partial_start(next);
			}
		}	
	}

	void add_partial_start(Tracker par_start)
	{
		partial_starts.push_back(par_start);
	}

private:
	vector<Tracker> partial_starts;
	Tracker start_pos;
};

class PDBSeq: public PDB <SeqProj>
{
public:
	virtual void init(TransactionContainer & db) //should only called by root node initialization
	{
		for(int i = 0; i < db.size(); i++)
		{
			projDB.push_back(new Proj(db[i], i));
		}
	}


	virtual void scan_project (PDB_Map & new_PDB, bool is_root) 
	{
		ProjT_Map new_projs;
		for(int i = 0; i < projDB.size(); i++)
		{
			//cout<<"slave "<<_my_rank<<"before project"<<endl;
			projDB[i]->project(new_projs);
			//cout<<"slave "<<_my_rank<<"after project"<<endl;

			if(new_projs.empty())
				continue;
			
			ProjT_MapIter it = new_projs.begin();
			for(; it != new_projs.end(); it++)
			{
				PDB_MapIter mit = new_PDB.find(it->first);
				if( mit == new_PDB.end())
				{
					//new_PDB[it->first]=new PDB();
					//mit = new_PDB.find(it->first);
					PDBSeq* tmp = new PDBSeq();
					tmp->add((Proj*)it->second);
					new_PDB[it->first] = tmp;
				}
				else
				{
					mit->second->add((Proj*)it->second);
				}
			}

			new_projs.clear();
		}
	}

	virtual int support()
	{
		return projDB.size();
	}
};

//string ==> type of items directly read from the file
//SeqTran ==> type of transactions after processing
//          Usually means the transaction type after calling mapping()
//          After pruning globally infrequent items
//SeqPatt ==> type of pattern
//SeqProj ==> type of projected transactions
class CharSeq : public Worker<int, PDBSeq >
{
private:
	RecordContainer original_trans;
	int customerID;

public:
	CharSeq():Worker()
	{
		customerID = -1;
	}

	virtual void to_record(char* line, RecordContainer & records)
	{
		char* pch;
		pch = strtok (line," ");//customer id
		int new_ID = atoi(pch);

		if(new_ID != customerID)
		{
			//new customer in local databas

			customerID = new_ID;
			//if(_my_rank == 2)
				//cout<<"customerID: "<<customerID<<endl;
			pch = strtok(NULL, " ");//itemset id

			pch = strtok(NULL, " ");//itemset size
			int size = atoi(pch);
			//if(_my_rank == 2)
				//cout<<"read in itemset size:"<<size<<endl;

			Record tgt, empty;
			pch = strtok(NULL," ");
			original_trans.push_back(Record() );
			Record& new_t = original_trans.back();
			while(pch != NULL)
			{
				//if(_my_rank == 2)
					//printf ("%s\n",pch);
				tgt.push_back(atoi(pch) + 1);
				new_t.push_back(atoi(pch) + 1);
    			pch = strtok (NULL, " ");
			} 
			records.push_back(empty);
			records.back().swap(tgt);
			original_trans.back().back() = -original_trans.back().back();

			/*
			if(_my_rank == 2){
				for(int i = 0; i < original_trans.back().size(); i++)
					cout<<original_trans.back()[i]<<":"<<records.back()[i]<<" ";
				cout<<endl;
			}
			*/
		}
		else
		{
			pch = strtok(NULL, " ");
			pch = strtok(NULL, " ");
			int size = atoi(pch);
			//if(_my_rank == 2)
				//cout<<"read in itemset size:"<<size<<endl;

			pch = strtok(NULL, " ");
			Record & r = records.back();
			Record & new_t = original_trans.back();
			while(pch != NULL)
			{
				//if(_my_rank == 2)
					//printf ("%s\n",pch);
				r.push_back(atoi(pch)+1);
				new_t.push_back(atoi(pch) + 1);
    			pch = strtok (NULL, " ");
			}
			original_trans.back().back() = -original_trans.back().back();

			/*
			if(_my_rank == 2){
				for(int i = 0; i < original_trans.back().size(); i++)
					cout<<original_trans.back()[i]<<":"<<records.back()[i]<<" ";
				cout<<endl;
			}
			*/
		}
	}

	struct icpair //item-count-pair
    {
    	ReadInItemT item;
    	int count;

    	inline bool operator<(const icpair& rhs) const
		{
			return count > rhs.count; //non-increasing order of freq
		}
    };


    //record => transaction: use itemlist
    void rec_to_trans(Record & r, Transaction & t, unordered_map<ReadInItemT, TItemT> & mapping)
    {
    	typename unordered_map<ReadInItemT, TItemT>::iterator mit;
    	
    	for(int i = 0; i < r.size(); i++)
    	{
    		ReadInItemT tmp = r[i] < 0 ? -r[i] : r[i];
    		mit = mapping.find(tmp);
    		if(mit != mapping.end())
    		{
    			t.push_back(r[i]);
    		} 
    		else if (r[i] < 0)
    		{
    			t.back() = -t.back();
    		}
    	}	
    }

	virtual void mapping(RecordContainer & records, TransactionContainer  & transactions, unordered_map<ReadInItemT, int> & itemlist)
    {
        //records => transactions: use itemlist
        //1. construct mapping[ItemT] = ItemID: use itemlist
        unordered_map<ReadInItemT, TItemT> mapping;

        typename ItemTable::iterator it;
    	vector<icpair> vec;
    	icpair tmp;
    	for(it = itemlist.begin(); it != itemlist.end(); it++){
    		tmp.item = it->first;
    		tmp.count = it->second;
    		vec.push_back(tmp);
    	}
    	sort(vec.begin(), vec.end());

        for(int i=0; i<vec.size(); i++) 
        {
        	mapping[vec[i].item] = (TItemT)i+1; //avoid 0
        }

        //2. records => transactions: use mapping
        Transaction t, empty;
        for(int i=0; i<original_trans.size(); i++)
        {
            rec_to_trans(original_trans[i], t, mapping);
            records[i].clear(); //r => t, r is no longer needed, freed to save space
            original_trans[i].clear();

            if(t.size() > 0) //remove empty sequence
            {
                transactions.push_back(empty);
                transactions.back().swap(t);
            }
        }

        /*
        if(_my_rank == 3)
        {
        	cout<<"after mapping: "<<endl;
        	for(int i = 0; i < transactions.size(); i++)
        	{
        		Transaction & t = transactions[i];
        		for(int j = 0; j < t.size(); j++)
        			cout<<t[j]<<" ";
        		cout<<endl;
        	}
        }
        */
    }
};

int main(int argc, char* argv[]){
	CharSeq worker;
	WorkerParams params;
	params.input_path = "/yanda/toy3";
	params.threshold = 0.3244;
	worker.run(params);
	return 0;
}
