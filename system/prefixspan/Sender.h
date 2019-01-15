#ifndef SENDER_H
#define SENDER_H

#include "msg.h"

template<class PattT>
class MasterSender{
	typedef vector<Message<PattT> > MsgContainer;
	
	MsgContainer send_buf;
	char fname[100];

	int numfile_sent;
	vector<int> batch_sent;
	int cur_pos;

	void send_stream()
	{
		send_buf.clear();
		set_fname(fname, cur_pos, batch_sent[cur_pos] ++);
		//cout<<"send file: "<<fname<<endl;
		ifbinstream in(fname);
		Message<PattT> empty;
		while(!in.eof())
		{
			send_buf.push_back(empty);
			in >> send_buf.back();
		}
		send_data(send_buf, cur_pos+1);
		in.close();
		numfile_sent ++;
		remove(fname);  //delete the file after sending
	}

	//wait until notified by another thread
	void wait_for_batch()
	{
		//cout<<"~~~~wait~~~~"<<endl;
		unique_lock<mutex> lock_batch(mtx_batch);
		while(numfile_written == numfile_sent && !no_more_file) cond_batch.wait(lock_batch);
		//for(int i = 0; i < batch_written.size(); i++) cout<<batch_written[i]<<" ";
		//cout<<endl;
		//cout<<"weaken!!"<<endl;
	}

	//update the cur_pos to the position which has new file to send
	//if find such a position, returen true; otherwise, return false
	bool scan() //only run
	{
		int tgt = (cur_pos+1)%_num_miners;

		int cnt = 0;
		while (batch_written[tgt] == batch_sent[tgt])
		{
			tgt++;
			if(tgt >= _num_miners) tgt -= _num_miners;
			//stop after a round, and start "tgt" still has nothing to send
			cnt++;
			if(cnt > _num_miners) return false;
			
		}
		cur_pos = tgt;
		return true;
	}


public:
	MasterSender(): batch_sent(_num_miners, 0){
		numfile_sent = 0;
		cur_pos = -1;
	}

	void run_send()
	{
		while(true)
		{
			wait_for_batch(); // wait until another thread create new batch on disk
			while(scan()) send_stream();
				//cout<<"sending to"<<cur_pos<<endl;
			if(no_more_file && numfile_written == numfile_sent){
				break;
			}
		}
		//cout<<"sender end!!"<<endl;

		//rmdir(SENDER_DIR.c_str());
	}

		
	

};

#endif