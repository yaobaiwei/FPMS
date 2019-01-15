#ifndef PDB_H
#define PDB_H

#include "ProjTran.h"

template <class ProjTranT>
class PDB{

public:
	typedef typename ProjTranT::ItemType ItemT;
	typedef ItemT ItemType; // for pass to class using PDB
	typedef typename ProjTranT::TranType TranT;
	typedef TranT TranType; // for pass to class using PDB
	typedef typename ProjTranT::DeltaType DeltaT;
	typedef DeltaT DeltaType; // for pass to class using PDB
	typedef typename ProjTranT::PattType PattT;
	typedef PattT PattType;
	typedef ProjTranT ProjTranType;


	typedef typename ProjTranT::ProjT_map ProjT_Map;
	typedef typename ProjT_Map::iterator ProjT_MapIter;


	
	typedef vector<TranT> TransactionContainer;
	typedef ProjTranT Proj;
	typedef unordered_map<DeltaT, PDB*> PDB_Map;
	typedef typename PDB_Map::iterator PDB_MapIter;

	typedef typename  vector<Proj*>::iterator PDBIter;

    vector<Proj*> projDB; //projected transactions


	//PDB(){}
	
	virtual void init(TransactionContainer & db) = 0; //should only called by root node initialization
	/*
	{
		for(int i = 0; i < db.size(); i++)
		{
			projDB.push_back(new Proj(db[i], i));
		}
	}
	*/

	virtual void scan_project (PDB_Map & new_PDB, bool is_root) = 0;
	/*
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
					PDB<Proj>* tmp = new PDB<Proj>();
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
	*/
	
	
	//UDF
	virtual int support() = 0;
	/*
	{
		return projDB.size();
	}
	*/

	int size()
	{
		return projDB.size();
	}

	~PDB()
	{
		for(int i = 0; i < projDB.size(); i++)
			delete projDB[i];
	}

	void add(Proj* tran)
	{
		projDB.push_back(tran);
	}

};

#endif