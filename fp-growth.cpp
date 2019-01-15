#include "prefixspan/Worker.h"
#include <cstring>

//An example of prefixspan
//ItemID = short

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
		new_patt.push_back(d);

		new_patt.next = new_patt.begin();
	}

	virtual inline void print()
	{
		for(int i = 0; i < this->size(); i++)
			cout<<at(i)<<" ";
	}
};

//Transaction type: containing transactions after pruning globally infrequent items;
//					directly consist of database used in frequent pattern mining
//					!!! must expose iterator to pubilc
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

	virtual void project(ProjT_map & tmp)
	{
		if (start_pos == transaction.end()) return;
		for(Tracker it = start_pos; it != transaction.end(); it++)
		{
			DeltaT item = *it;
			if(tmp.find(item) == tmp.end())
			{
				
				Tracker next = it;
				next ++;
				tmp[item] = new SeqProj(transaction, tran_id, next);
				//assert(tmp[item]);
			}
		}

	}

private:
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
class CharSeq : public Worker<string, PDBSeq >
{
public:
	virtual void to_record(char* line, RecordContainer & records)
	{
		Record tgt, empty;
		char* pch;
		pch = strtok (line,",");
		while(pch != NULL)
		{
			//printf ("%s\n",pch);
			tgt.push_back(string(pch));
    		pch = strtok (NULL, ",");
		} 
		records.push_back(empty);
		records.back().swap(tgt);
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
    	typename unordered_map<ReadInItemT, TItemT>::iterator it;
    	for(int i=0; i<r.size(); i++)
    	{
    		it = mapping.find(r[i]);
    		if(it != mapping.end()) t.push_back(it->second);
    	}
    }

	virtual void mapping(RecordContainer & records, TransactionContainer  & transactions,  unordered_map<ReadInItemT, int> & itemlist)
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
        	mapping[vec[i].item] = (TItemT)i; 
        	//if(_my_rank == 3)
        		//cout<<vec[i].item<<": "<<mapping[vec[i].item]<<endl;
        }
        
        //2. records => transactions: use mapping
        Transaction t, empty;
        for(int i=0; i<records.size(); i++)
        {
            rec_to_trans(records[i], t, mapping);
            records[i].clear(); //r => t, r is no longer needed, freed to save space
            if(t.size() > 0) //remove empty sequence
            {
                transactions.push_back(empty);
                transactions.back().swap(t);
            }
        }
    }
};

int main(int argc, char* argv[]){
	CharSeq worker;
	WorkerParams params;
	params.input_path = "/yanda/toy2";
	params.threshold = 0.3254;
	worker.run(params);
	return 0;
}
