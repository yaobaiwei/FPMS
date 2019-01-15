#ifndef SLAVE_H
#define SLAVE_H


#include "SlaveNode.h"
#include "msg.h"

template<class SNodeT>
class Slave{

public:
	typedef SNodeT SlaveNode;
	typedef typename SNodeT::Delta DeltaT;
	typedef typename SNodeT::Pattern PattT;
	typedef typename SNodeT::Grown_Map Children_Map;
	typedef typename SNodeT::Grown_MapIter Children_MapIter;
	typedef typename SNodeT::Transactions TransactionContainer;

	bool active_finish; // finished mining local freq patterns?
	RecvBuf<PattT> recv_buf;
	SlaveSendBuf<PattT> send_buf;

	SlaveNode* min_frontier;
	//TransactionContainer & db;

	Slave(TransactionContainer & transactions)
	{
		root = new SlaveNode(transactions);
		active_finish = false;
		//min_frontier = root;
	}

	/**********************************************************************************/
	/*First stage: when slave notices that no message needs to process, it will go on */
	/*finding new locally frequent pattern; otherwise it will process the messages in */
	/*the buffer first (either requests or confirms)                                  */
	/*                                                                                */
	/*Second stage: after slave has already finished mining all locally frequent      */ 
	/*patterns, slave will keep waiting for master's request/confirm until it receives*/ 
	/* an end signal from master.                                                     */
	/**********************************************************************************/
	void run()  
	{
		//stage1
		
		while(true)
		{
			if(!process_msg())
				break;
			if (!active_finish)//check whether still need active mining
			{
				SlaveNode* frontier = trace(root);
				//trace will return the node that can grow if there is any
				//otherwise it will return null

			//cout<<"slave "<<_my_rank<<">>>>>>>>>>>>>>>trace"<<endl;

				if(!frontier)
				{
					//cout<<"slave"<<_my_rank<<"finished active mining"<<endl;
					active_finish = true;
					Message<PattT> msg;
					msg.freq = JOB_END;
					send_buf.add(msg);
					continue;
				}

		
			//cout<<"slave "<<_my_rank<<">>>>>>>>>>>>>>>if"<<endl;
			/*
				if(_my_rank == 1)
				{
					cout<<"return from trace: ";
					frontier->pattern.print();
					cout<<endl;
				}
			*/
				
				Children_MapIter mit = frontier->get_next();
				if(mit == frontier->children.end())
					continue;

		
			//cout<<"slave "<<_my_rank<<">>>>>>>>>>>>>>>itemT"<<endl;
				
				SlaveNode* child = frontier->grow(mit);
				

		
			//cout<<"slave "<<_my_rank<<">>>>>>>>>>>>>>>child"<<endl;

				report(child);

				frontier->update_next();

		
			//cout<<"slave "<<_my_rank<<">>>>>>>>>>>>>>>next"<<endl;

				
				//if(_my_rank != 0)
				//{
				//	cout<<"slave "<<_my_rank<<" grow new pattern:";
				//	for(int i = 0; i < child->pattern.size(); i++)
				//		cout<<child->pattern[i]<<" ";
				//	cout<<endl;
				//}
				
			}

			if(send_buf.timeout())
				send_buf.flush();
		}

		//cout<<"slave end"<<endl;
	}

	~Slave()
	{
		delete root;
	}

private:
	SlaveNode* root;

	//process_msg will return false, if it receive master message JOB_END,
	//which indicates that master tells all slaves they can stop.
	//Otherwise, process_msg always return true when it finishes processing all existing msgs
	bool process_msg()
	{
		Message<PattT> msg;
		bool has_msg = recv_buf.unblock_nextMsg(msg);
		while(has_msg)
		{
			//if(_my_rank != 0)
			//{
				//cout<<"slave "<<_my_rank<<" receive msg:";
				//for(int i = 0; i<msg.pattern.size(); i++)cout<<msg.pattern[i]<<" ";
				//cout<<": "<<msg.freq<<endl;
			//}

			if (msg.freq == JOB_END)//master tells slave to stop
				return false;

			//cout<<"slave "<<_my_rank<<" before match"<<endl;
			SlaveNode* node;
			msg.pattern.init_start();
			node = match(msg.pattern, root);
			//cout<<"slave "<<_my_rank<<" after match"<<endl;

			if(node != NULL)
			{
				int diff = msg.pattern.size() - node->pattern.size();

				if(msg.freq == REQUEST) //for request msg
				{
					//cout<<"slave "<<_my_rank<<" receive req:";
					//for(int i = 0; i<msg.pattern.size(); i++)cout<<msg.pattern[i]<<" ";
					//cout<<endl;
					if (diff == 1)
					{
						Children_MapIter it = node->children.find(msg.pattern.back());
						SlaveNode* child;
						//assert(it->second.child == NULL);

						if(it != node->children.end())
						{
							//cout<<"slave "<<_my_rank<<" grow"<<endl;
							child = node->grow(it);
							report(child);
						}	
						else
						{
							//cout<<"slave "<<_my_rank<<" grow zero"<<endl;
							//child = node->grow_zero(msg.pattern.back());
							report_not_found(msg.pattern);
						}

						//cout<<"slave "<<_my_rank<<" in request2"<<endl;
						//node->update_next_items(msg.pattern.back()); //update the set 
						//cout<<"slave "<<_my_rank<<" in request3"<<endl;
					}
					else
					{
						report_not_found(msg.pattern);
					}
				}
				else if(msg.freq == MIN_FRONTIER)
				{
					//cout<<"/////////////garbage collection"<<endl;
					
					if(msg.pattern.size() == 0)
						min_frontier = NULL;
					else
						min_frontier = node;
					delete_confirmed_node(root);
					
				}
				else //for confirm msg
				{
					//assert(node);
					if (diff > 0)
					{
						has_msg = recv_buf.unblock_nextMsg(msg);
						continue;
					}
					else if(msg.freq  == INFREQ) // if infrequent, delete it
					{
						delete_subtree(node);
						delete_from_parent(node);
					}
					else  // if frequent, update glob_freq
						node->glob_freq = msg.freq;
				}
			}
			
			has_msg = recv_buf.unblock_nextMsg(msg);
		}

		return true;
	}

	/*
	void print_tree(SlaveNode*root, int level)
	{
		cout<<level<<"-";
		for(int i = 0; i<root->pattern.size(); i++) cout<<root->pattern[i];
		cout<<endl;
		for(int i = 0; i < root->children.size(); i++)
		{
			if (root->children[i])
				print_tree(root->children[i], level+1);
		}
	}
	*/
	
	SlaveNode* trace(SlaveNode* curNode)
	{
		if(curNode == NULL) //if curNode is deleted, return NULL
			return NULL;
		if(!curNode->local_freq() || !curNode->can_grow()) //if curNode is locally infrequent, return NULL
			return NULL;
		if(curNode->not_yet_grown()) // if curNode has no children
			return curNode;

		//recursively call on the righ most children
		Children_MapIter it = curNode->get_cur();
		SlaveNode* tmp = trace(it->second.child);

		if(!tmp) //if none of the descendants on the righ most branch can grow new
		{
			if (curNode->all_grown()) //if curNode has explored all possible children, return NULL
				return NULL;
			else //if curNode can explore some other children, return curNode
				return curNode;
		}
		else //if some of the descendants on the righ most branch can grow, return it
			return tmp;
	}


	SlaveNode* match(PattT &p, SlaveNode* curNode) //same as the one of master
	{
		int level = curNode->pattern.size();
		int plen = p.size();
		//assert(level <= plen);
		if (level == plen) return curNode;
		else //level < plen
		{
			//level = curNode's children's last element's position
			DeltaT next_delta = p.next_delta();
			Children_MapIter it = curNode->children.find(next_delta);

			if(it == curNode->children.end())//pattern not exist in local DB
				return curNode;
			else if (it->second.child) //the matching child has been grown
				return match(p, it->second.child); //the matching child exists, go to that child
			else 
				return curNode; //the matching child has not been grown yet, return the parent
		}
	}

	void delete_subtree(SlaveNode* curNode)
	{
		Children_MapIter it = curNode->children.begin();

		for(; it != curNode->children.end(); it++)
		{
			if(it->second.child != NULL)
			{
				delete_subtree(it->second.child);
				delete it->second.child;
			}
		}
	}

	void delete_from_parent(SlaveNode* curNode)
	{
		SlaveNode* parent = curNode->parent;
		DeltaT last = curNode->pattern.back();
		Children_MapIter mit = parent->children.find(last);

		//assert(mit != parent->children.end());
		mit->second.child = NULL;
		parent->children_rest --;
		//assert(parent->children_rest >=0);
		delete curNode; 
	}

	void delete_confirmed_node(SlaveNode* curNode)
	{
		dfs_scan(curNode);
	}

	bool dfs_scan(SlaveNode* curNode)
	{
		//min_frontier==NULL means all slaves have finished active mining,
		//therefore master no longer need boundary when doing dfs.
		//min_frontier!=NULL means some slaves have not finished, 
		//So when it hits the boundary, return false back to stop the recursion.
		if(min_frontier && curNode == min_frontier)
			return false;

		if(curNode->confirmed() || curNode->parent == NULL)
		{
			SlaveNode* child;
			bool go_on = true;
			Children_MapIter it = curNode->children.begin();
			for(; it != curNode->children.end(); it++)
			{
				child = it->second.child;
				//child==NULL means the child has been deleted
				if(child)
					go_on = dfs_scan(child); 
				//go_on==false means it has hit boundary
				if(! go_on)
					return false;
			}
			if(curNode->children_rest == 0 && curNode->confirmed() && curNode != min_frontier)
				delete_from_parent(curNode);
			return true;
		}
		else 
			return true;
	}

	void report(SlaveNode* node)
	{
		Message<PattT> msg;
		
		msg.pattern = node->pattern;

		msg.freq = node->loc_freq;

		/*
		if(_my_rank == 1){
			cout<<"slave"<<_my_rank<<" report ";
			for(int i = 0;i < msg.pattern.size(); i++) cout<<msg.pattern[i]<<" ";
			cout<<" "<< msg.freq <<endl;
		}
		*/

		send_buf.add(msg);
		//sleep(1);
	}

	void report_not_found(PattT & p)
	{
		Message<PattT> msg;
		
		msg.pattern = p;

		msg.freq = 0;

		/*
		if(_my_rank == 1){
			cout<<"slave"<<_my_rank<<" report ";
			for(int i = 0;i < msg.pattern.size(); i++) cout<<msg.pattern[i]<<" ";
			cout<<" "<< msg.freq <<endl;
		}
		*/

		send_buf.add(msg);
		//sleep(1);
	}
};

#endif