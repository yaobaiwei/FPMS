#ifndef SLAVENODE_H
#define SLAVENODE_H


#include "PDB.h"

#define UNCONFIRMED -1

template <class PattT, class PDBT>
class SlaveNode{
public:
	typedef PattT Pattern;
	typedef PDBT PDB;
	typedef typename PDB::DeltaType Delta;
	typedef typename PDB::PDB_Map PDB_map;
	typedef typename PDB_map::iterator PDB_mapIter;
	typedef typename PDB::TransactionContainer Transactions;

	typedef struct PCpair
	{
		PDB* pdb;
		SlaveNode* child;
	} PDB_child;

	typedef map<Delta, PDB_child> Grown_Map;
	typedef typename Grown_Map::iterator Grown_MapIter;
	

	Pattern pattern;
	long long loc_freq;
	PDB* pdb;
	long long glob_freq; // will be UNCONFIRMED if master haven't confirm the status of this node
	
	int children_total;
	int children_rest;
	SlaveNode* parent;

	Grown_Map children;
	//set<ItemT> next_items; 
	Grown_MapIter cur, next;
	bool has_candidate;
	

	SlaveNode(Transactions & transactions) //for building root 
	{
		//build origin PDB;
		pdb = new PDB();
		pdb->init(transactions);
		loc_freq = pdb->support();
		glob_freq = UNCONFIRMED;
		parent = NULL;
		has_candidate = false;
		//first project, generate map
		//cout<<">>>>>>>>>>>>initial scan"<<endl;
		PDB_map pdbs;
		pdb->scan_project(pdbs, true);
		//cout<<">>>>>>>>>>>>after initial scan:"<<loc_freq<<endl;

		PDB_mapIter it = pdbs.begin();
		//cout<<"children of root:";
		for(;it != pdbs.end(); it++)
		{	
			children[it->first] = {(PDB*)it->second, NULL};
			//cout<<it->first<<", ";
			//if(_my_rank == 3)
				//cout<<"slave 3 grow: "<<it->first<<"~"<<endl;
			if(!has_candidate && it->second->support() >= req_count)
				has_candidate = true;
		}
		//cout<<endl;
		next = children.begin();
		cur = children.end();
		if(next != children.end() && next->second.pdb->support() < req_count)
			update_next();

		//--debug
		//if(_my_rank == 1)
		//	cout<<"after initial, set size:"<<next_items.size()<<endl;

		children_total = 0;
		children_rest = 0;
	}

	SlaveNode(Grown_MapIter it, SlaveNode* parent_node) //for building other root
	{	
		//cout<<"it->first: "<<it->first<<endl;
		parent_node->pattern.grow(it->first, pattern);

		pdb = it->second.pdb;
		loc_freq = pdb->support();

		glob_freq = UNCONFIRMED;

		parent = parent_node;
		has_candidate = false;
		/*
		if(_my_rank != 0){
			cout<<"slave "<<_my_rank<<" when grow ";
			pattern.print();
			cout<<"==>"<<loc_freq;
			cout<<endl;
		}
		*/
		
		PDB_map pdbs;
		pdb->scan_project(pdbs, false);

		
		//cout<<"slave "<<_my_rank<<" scan and proj"<<endl;

		PDB_mapIter mit = pdbs.begin();
		for(;mit != pdbs.end(); mit++)
		{	
			children[mit->first] = {(PDB*)mit->second, NULL};
			//if(_my_rank == 3)
				//cout<<"slave 3 grow: "<<mit->first<<"~"<<mit->second->support()<<endl;
			if(!has_candidate && mit->second->support() >= req_count)
				has_candidate = true;
		}
		//if(_my_rank != 0)
		//	cout<<"slave "<<_my_rank<<" after foor loop"<<endl;
		next = children.begin();
		cur = children.end();
		if(next != children.end() && next->second.pdb->support() < req_count)
			update_next();

		//if(_my_rank != 0)
		//	cout<<"slave "<<_my_rank<<"set size: "<<next_items.size()<<endl;

		children_total = 0;
		children_rest = 0;

	}

	SlaveNode* grow(Grown_MapIter it)
	{
		//assert(it != children.end());

		//cout<<"slave "<<_my_rank<<" before grow: ";
		//for(int i = 0; i<new_p.size(); i++) cout<<new_p[i]<<" ";
		//cout<<" parent node:";
		//for(int i = 0; i<pattern.size(); i++) cout<<pattern[i]<<" ";
		//cout<<" PDB size:" <<it->second.first->support()<<endl;
		//cout<<endl;

		SlaveNode* child = new SlaveNode(it, this);
		//cout<<"slave "<<_my_rank<<" after grow"<<endl;
		it->second.child = child;
		children_total ++;
		children_rest ++;

		return child;
	}

	/*

	SlaveNode* grow_zero(ItemT item)
	{
		Pattern new_p = pattern;
		new_p.push_back(item);

		PDB* new_pdb = new PDB();
		SlaveNode* child = new SlaveNode(new_p, new_pdb, this);
		children[item] = make_pair(new_pdb, child);
		children_total ++;
		children_rest ++;
	}

	*/

	inline bool all_grown()
	{
		return next == children.end();
	}


	inline bool not_yet_grown()
	{
		return children_total == 0 && loc_freq >= req_count;
	}

	inline bool local_freq()
	{
		return loc_freq >= req_count;
	}

	inline bool can_grow()
	{
		return has_candidate;
		//return !children.empty();
		//return (!children.empty()) && next != children.end();
	}

	inline bool confirmed()
	{
		return glob_freq != UNCONFIRMED;
	}

	inline void update_next()
	{
		if(next != children.end()){
			if(next->second.pdb->support() >= req_count)
				cur = next;
			for(next ++ ; next != children.end() && next->second.pdb->support() < req_count; next ++);
		}
	}

	inline Grown_MapIter get_next()
	{
		//if(next->second.pdb->support() < req_count)
			//for(next ++ ; next != children.end() && next->second.pdb->support() < req_count; next ++);
		return next;
	}

	inline Grown_MapIter get_cur()
	{
		return cur;
	}

	~SlaveNode()
	{
		//cout<<"slave "<<_my_rank<<"-------delete: ";
		//for(int i = 0; i < pattern.size(); i++)
		//	cout<<pattern[i]<<" ";
		//cout<<endl;

		Grown_MapIter mit;
		for(mit = children.begin();mit != children.end(); mit++)
			delete mit->second.pdb;
		if (parent == NULL)
			delete pdb;
	}
};	

#endif