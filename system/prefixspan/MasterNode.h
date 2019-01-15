#ifndef MASTERNODE_H
#define MASTERNODE_H

#include "../utils/global.h"
#include <list>
#include <map>

template<class PatternT>
class MasterNode{
public:
	typedef PatternT Pattern; 
	typedef typename PatternT::DeltaType Delta;
	typedef map<Delta, MasterNode*> Children_Map;

	Pattern pattern; //pattern of the node
	vector<long long> freq_array; //local freqencies from each slave
	int freq_count; //how many slaves have report loc_freq
	long long freq_sum; //total frequency
	int children_total; //how many children have been grown
	int children_rest; // how many children exist now

	//vector<MasterNode*> children;
	Children_Map children;
	MasterNode* parent;    //when parent == NULL, it must be root node 

	bool requested;
	//list<MasterNode*>::iterator pos;

	MasterNode()
	{
		freq_count = _num_miners;
		freq_sum = 0;
		children_total = 0;
		children_rest = 0;
		parent = NULL;
		num_MasterNode ++;

		requested = true;
		//pos = list.end();
	}
	MasterNode(Pattern& p, MasterNode* node):freq_array(_num_miners,NA)
	{
		pattern = p;
		freq_count = 0;
		freq_sum = 0;
		children_total = 0;
		children_rest = 0;
		parent = node;
		num_MasterNode ++;

		requested = false;
		//pos = list.end();
	}

	void add_local_freq(int src, int loc_freq)
	{
		freq_array[SLAVE_ID2POS(src)] = loc_freq;
		freq_sum += loc_freq;
		freq_count ++;
	}

	MasterNode* add_child(Pattern& p)
	{
		MasterNode* child = new MasterNode(p, this);
		children[p.back()] = child;
		//cout<<"current pattern: ";
		//p.print();
		//cout<<endl;
		//cout<<"p.back(): "<<p.back()<<endl;
		//children[p.back()] = child;
		children_total ++;
		children_rest ++;

		//for parameter adjustment
		
		if (num_MasterNode > max_num_MasterNode)
			max_num_MasterNode = num_MasterNode;

		return child;
	}

	bool glob_freq()
	{
		return freq_sum >= req_count;
	}

	bool confirmed()
	{
		return freq_count >= _num_miners;
	}

	void delete_child(Pattern& p)
	{
		Delta pos = p.back();
		children[pos] = NULL;
		children_rest --;
	}

	~MasterNode()
	{
		num_MasterNode --;
	}
};

#endif