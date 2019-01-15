#ifndef MASTER_H
#define MASTER_H

#include "MasterNode.h"
#include "../utils/Heap.h"
#include "Sender.h"
#include <stack>
#include <thread>


#define REQ_LIMIT 100
#define SCAN_TIMEOUT 0.1
#define MSG_TIMEOUT 0.12

template <class MNodeT>
struct NodeInfo
{
	int src;
	MNodeT* node;
};

template <class MNodeT>
class Master{
public:
	typedef MNodeT MasterNodeT;
	typedef typename MasterNodeT::Pattern PattT;
	typedef typename MasterNodeT::Children_Map CMap;
	typedef typename MasterNodeT::Delta DeltaT;

	typedef typename CMap::iterator CMap_Iter;
	typedef struct NodeInfo<MasterNodeT> NodeInfo;

	MasterNodeT* root;

	heap<PattT, NodeInfo> hp; //each elements has format <pattern, slaveID>, for selecting the min_frontier
	vector<qelem<PattT, NodeInfo> > frontiers; //record the progress of each slave
	//add for new strategy
	int active_mining_finish;
	MasterNodeT* min_frontier;//if min_frontier == NULL, that means no boundary
	//MasterNode* upper_bound;
	double last_req_time, last_send_time;
	int req_num;
	//double last_print_tree;//for debug

	RecvBuf<PattT> recv_buf;
	MasterSendBufs<PattT> send_buf;
	MasterSender<PattT> sender;

	Master():frontiers(_num_miners){
		root = new MasterNodeT();
		min_frontier = root;

		for(int i = 0; i < _num_miners; i++)
		{
			frontiers[i].key = PattT();
			frontiers[i].val.src = SLAVE_POS2ID(i);
			frontiers[i].val.node = NULL;
			hp.add(frontiers[i]);
		}

		last_req_time = get_current_time();
		last_send_time = get_current_time();
		//last_print_tree = get_current_time();
		req_num = 0;
		active_mining_finish = 0;
	}
	~Master(){
		//delete root;
	}

	void run(){
		thread send_thread(&MasterSender<PattT>::run_send, &sender);
		run_comp();

		send_thread.join();
	}
	
	
	void run_comp(){
		Message<PattT> msg;
		int src;
		
		//cout<<"element size: "<<elements_size<<endl;
		while(root->children_total < elements_size || root->children_rest != 0)
		{
			//1. receive messages
			if(!recv_buf.unblock_nextMsg(msg,src))
			{
				if(get_current_time()-last_req_time > SCAN_TIMEOUT){
					scan_tree();
					last_req_time = get_current_time();
				}
				if(get_current_time()-last_send_time > MSG_TIMEOUT){
					//print_tree(root,0);
					send_buf.flush_all();
					last_send_time = get_current_time();
				}
				//debug
				//if(get_current_time()-last_print_tree > 2){
					//print_tree(root,0);
					//last_print_tree = get_current_time();
				//}
				continue;
			}

			//debug
			//last_print_tree = get_current_time();

			if(msg.freq == JOB_END)
			{
				active_mining_finish ++;
				//set min_frontier to the end after all slaves finish active mining
				//if(active_mining_finish >= _num_miners)
					//min_frontier = NULL;
				continue;
			}

			 //print out new msg for debug
			//cout<<"slave"<<src<<": ";
			//msg.pattern.print();
			//for(int i = 0; i < msg.pattern.size(); i++)cout<<msg.pattern[i]<<" ";
			//cout<<"~"<<msg.freq<<endl;
			
			
			//2. update the tree 
			MasterNodeT* tgt = match(msg.pattern);
			if(! tgt)
			{
				//cout<<"abnormal msg: ";
				//for(int i = 0; i<msg.pattern.size(); i++)cout<<msg.pattern[i]<<" ";
				//cout<<endl;
				continue;
			}
			
			if(msg.pattern.size() - tgt->pattern.size() == 1)
			{
				tgt = tgt->add_child(msg.pattern); 	
			}
			
			tgt->add_local_freq(src, msg.freq);
			
			
			//3. update the min_frontier
			if(active_mining_finish < _num_miners)
				update_frontier(msg.pattern, src, tgt);
			

			//4.confirm the node and try to shrink the tree
			if(tgt->confirmed())
			{
				//send confirms
				if(! tgt->glob_freq()) //if it is infrequent node
				{	
					//output infrequent pattern in terms of ItemID
					//cout<<"----infrequent: ";
					//for(int i = 0; i< tgt->pattern.size(); i++) cout<<tgt->pattern[i];
					//cout<<endl;
					confirm(msg.pattern, INFREQ);
					//delete the subtree imediately
					delete_subtree(tgt);
					//delete tgt itself
					delete_from_parent(tgt);
				}
				else
				{
					//output frequent pattern in terms of ItemID
					confirm(msg.pattern, tgt->freq_sum);

					cout<<"****frequent: ";
					tgt->pattern.print();
					//for(int i = 0; i< tgt->pattern.size(); i++) cout<<tgt->pattern[i]<<" ";
					//for(int i = 0; i< tgt->pattern.size(); i++) cout<<itemlist[tgt->pattern[i]]<<" ";
					cout<<": "<< tgt->freq_sum <<endl;
				}

			}
			
			//5. scan the tree and send reqest 
			if(get_current_time() - last_req_time > SCAN_TIMEOUT)
			{
				scan_tree();
				last_req_time = get_current_time();
			}
			//cout<<"after a loop"<<endl;
			//print_tree(root, 0);
			
		}
		send_buf.set_end();
		cout<<"master finished"<<endl;
	}

	//for debug
	void print_tree(MasterNodeT*root, int level)
	{
		for(int i=0; i<level; i++) cout<<"-";
		root->pattern.print();
		cout<<endl;
		CMap_Iter it = root->children.begin();
		for(; it != root->children.end(); it++)
		{
			if (it->second)
				print_tree(it->second, level+1);
		}
	}

	//update the slowest slave's progress
	// update the heap
	bool update_frontier(PattT& pattern, int src, MasterNodeT* tgt)
	{
		//only when the pattern is greater than the existing one, we update heap
		if(pattern > frontiers[SLAVE_ID2POS(src)].key)
		{
			qelem<PattT, NodeInfo>& tmp = frontiers[SLAVE_ID2POS(src)];
			tmp.key = pattern;
			tmp.val.node = tgt;
			hp.fixHeap();
			//int index = SLAVE_ID2POS(hp.peek().val.src);
			//hp.remove();
			//hp.add(frontiers[index]);

			/*
			//update the min_frontier
			if(hp.peek().val.node)//if all slaves have reported some pattern
			{	
				min_frontier = hp.peek().val.node;
				//upper_bound = min_frontier;//upper bound for scan
				//while(upper_bound->parent && !upper_bound->parent->confirmed())
					//upper_bound = upper_bound->parent;
			}
			*/
		}

		return false;
	}

	//send request to some slaves, when we find NA on coresponding element on freq_array
	void request(MasterNodeT* node) 
	{
		//cout<<"send request"<<endl;
		vector<long long>& freq_array = node->freq_array;
		Message<PattT> msg;
		msg.pattern = node->pattern;
		msg.freq = REQUEST;
		
		//cout<<"request:";
		//for(int i =0; i<node->pattern.size(); i++)cout<<node->pattern[i]<<" ";
		//cout<<endl;
		
		for(int i = 0; i < freq_array.size(); i++)
		{
			if (freq_array[i] == NA) //NA == -1
			{
				send_buf.add_to(msg, i);
				total_request ++;
				//cout<<i<<"+";
			}
		}
		last_send_time = get_current_time();
		//cout<<endl;
		node->requested = true;
		
		
	}

	//if the pattern is globally frequent, send freq;
	//otherwise, send message with pattern and freq = INFREQUENT
	void confirm(PattT& p, long long freq) //send confirm to all slaves
	{
		Message<PattT> msg;
		msg.pattern = p;
		msg.freq = freq;

		send_buf.add_to_all(msg);
		//cout<<"add to all"<<endl;
	}

	void bcast_min_frontier(MasterNodeT* node)
	{
		Message<PattT> msg;
		if(node)
		{
			msg.pattern = node->pattern;
			msg.freq = MIN_FRONTIER;
		}
		else
		{
			msg.pattern = PattT();
			msg.freq = MIN_FRONTIER_TO_END;
		}

		send_buf.add_to_all(msg);
	}

	//return value of match() function:
	//1.return the exact matched node if there is such a node
	//2.return the parent of new node that hasen't been grown
	//3.return NULL in other cases
	MasterNodeT* match(PattT &p)
	{
		p.init_start();
		return match(p, root);
	}
	
	MasterNodeT* match(PattT &p, MasterNodeT* curNode) //assume p already matches the path till curNode
	{
		int level = curNode->pattern.size();
		int plen = p.size();
		if (level == plen) return curNode;
		else //level < plen
		{
			//level = curNode's children's last element's position
			DeltaT next_delta = p.next_delta();
			//cout<<"next delta: "<<next_delta<<endl;
			CMap_Iter it = curNode->children.find(next_delta);
			if (it != curNode->children.end()) //the matching child has been grown
			{
				if (it->second) return match(p, it->second); //the matching child exists, go to that child
				else return NULL; //the matching child has been freed, return NULL
			}
				else return curNode; //the matching child has not been grown yet, return the parent
		}
	}

	MasterNodeT* prefix_match(PattT &p, MasterNodeT* curNode)
	{
		int level = curNode->pattern.size();
		int plen = p.size();
		if(level == plen) return curNode;
		else if(!curNode->confirmed()) return curNode;//return when it meet unconfirmed node
		else
		{
			DeltaT next_delta = p.next_delta();
			//cout<<"delta: "<<next_delta<<endl;
			CMap_Iter mit = curNode->children.find(next_delta);
			//assert(mit != curNode->children.end());
			if(mit->second)return prefix_match(p, mit->second);
			else return curNode;
		}
	}

	void delete_subtree(MasterNodeT* curNode)
	{	
		CMap_Iter it = curNode->children.begin();

		for(; it != curNode->children.end(); it++)
		{
			MasterNodeT* tmp = it->second;
			if(tmp)
			{
				delete_subtree(tmp);
				delete tmp;
			}
			it->second = NULL;
		}
	}

	void delete_from_parent(MasterNodeT* curNode)
	{
		MasterNodeT* parent = curNode->parent;
		if(parent)
		{
			DeltaT last = curNode->pattern.back();
			CMap_Iter mit = parent->children.find(last);
			//assert(mit != parent->children.end());
			mit->second = NULL;
			parent->children_rest --;
			//assert(parent->children_rest >=0);
		}
		delete curNode; 
	}

	void scan_tree()
	{
		req_num = 0;
		//if not all slaves finish mining local frequent patterns, 
		//update the min_frontier by try to match pattern on the peek of the heap;
		//else, set min_frontier to NULL,
		//which means there is no min_frontier limitation any more.
		//cout<<"min_frontier: ";
		//hp.peek().key.print();
		//cout<<endl;
		//print_tree(root, 0);

		if(active_mining_finish < _num_miners)
		{
			hp.peek().key.init_start();
			min_frontier = prefix_match(hp.peek().key, root);
		}
		else min_frontier = NULL;
		dfs_scan(root);

		bcast_min_frontier(min_frontier);
	}

	bool dfs_scan(MasterNodeT* curNode)
	{
		//min_frontier==NULL means all slaves have finished active mining,
		//therefore master no longer need boundary when doing dfs.
		//min_frontier!=NULL means some slaves have not finished, 
		//master will not request ahead of slaves' mining progress.
		//So when it hits the boundary, return false back to stop the recursion.
		if(min_frontier && curNode == min_frontier)
			return false;
		if(req_num > REQ_LIMIT)
			return false;

		if(!curNode->confirmed())
		{
			if(!curNode->requested )//if curNode haven't confirmed nor requested
			{
				request(curNode);
				req_num ++;
				return true;
			}
			
		}
		else
		{
			MasterNodeT* child;
			bool go_on = true;
			CMap_Iter mit = curNode->children.begin();
			for(; mit != curNode->children.end(); mit++)
			{
				child = mit->second;
				//child==NULL means the child has been deleted
				if(child)
					go_on = dfs_scan(child); 
				//go_on==false means it has hit boundary
				if(! go_on)
					return false;
			}
			if(curNode->children_rest == 0 && curNode->confirmed())
			{
				//cout<<"======= master delete: ";
				//for(int i = 0; i < curNode->pattern.size(); i++)
					//cout<<curNode->pattern[i]<<" ";
				//cout<<endl;
				delete_from_parent(curNode);
				//cout<<"after delete"<<endl;
			}
			return true;
		}
	}	
};

#endif