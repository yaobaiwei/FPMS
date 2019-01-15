#ifndef MSG_H
#define MSG_H

#include <vector>
#include <queue>
#include <string.h>
#include "../utils/communication.h"
#include "../utils/ioser.h"
#include "../utils/timer.h"


void set_fname(char *fname, int dst, int batch_num){
	char num[20];
	strcpy(fname, SENDER_DIR.c_str());
	sprintf(num, "/%d", dst);
	strcat(fname, num);
	sprintf(num, "_%d",batch_num);
	strcat(fname, num);
}

template <class PattT>
class Message{
public:
	PattT pattern;
	long long freq; //may be REQUEST = -1 or JOB_END = -2


	friend ibinstream& operator>>(ibinstream& m, Message<PattT>& msg)
	{
		m >> msg.pattern;
    	m >> msg.freq;
    	return m;
	}

	friend obinstream& operator<<(obinstream& m, const Message<PattT>& msg)
	{
    	m << msg.pattern;
    	m << msg.freq;
    	return m;
	}

	friend ofbinstream& operator<<(ofbinstream& m, const Message<PattT>& msg)
	{
		m << msg.pattern;
		m << msg.freq;
		return m;
	}

	friend ifbinstream& operator>>(ifbinstream& m, Message<PattT>& msg)
	{	
		m >> msg.pattern;
		m >> msg.freq;
		return m;
	}
};

template <class PattT>
class SlaveSendBuf{
public:
	vector<Message<PattT> > msgs;
	double start_time;

	SlaveSendBuf(){}

	void add(Message<PattT>& msg)
	{
		push(msg);
		if(to_send())
		{
			//cout<<"slave"<<_my_rank<<" report"<<endl;
			send_msg_batch ++;
			send_data(msgs, MASTER_RANK);
			msgs.clear();
		}
	}

	void flush()
	{
		if(msgs.size() > 0)
		{
			send_data(msgs, MASTER_RANK);
			msgs.clear();
		}
	}

	~SlaveSendBuf(){
		if(msgs.size()>0)
			flush();
	}

	bool timeout(){
		double current = get_current_time();
		if(current - start_time >= TIMEOUT) return true;
		else return false;
	}

private:
	void set_time()
	{
		start_time = get_current_time();
	}

	void push(Message<PattT>& msg)
	{
		if(msgs.empty())
			set_time();
		msgs.push_back(msg);
	}

	bool to_send()
	{
		if(msgs.size() >= CAPACITY) return true;
		double current = get_current_time();
		if(current - start_time >= TIMEOUT) return true;
		return false;
	}
};

template <class PattT>
class MasterSendBufs{
public:
	typedef vector<Message<PattT> > MsgContainer;

	vector<MsgContainer> buffers;
	vector<double> start_times;
	vector<ofbinstream> streams;


	char fname[100];

	MasterSendBufs(): buffers(_num_miners), start_times(_num_miners), \
	streams(_num_miners){
		batch_written.resize(_num_miners, 0);
		/*
		for(int i = 0; i < _num_miners; i++){
			ofbinstream& m = streams[i];
			set_fname(fname, i, 0);
			m.open(fname);
		}
		*/

		no_more_file = false;
		_mkdir(SENDER_DIR.c_str());
	}

	~MasterSendBufs(){
		rmdir(SENDER_DIR.c_str());
	}

	void add_to(Message<PattT>& msg, int i)
	{
		//cout<<"add to "<<i<<endl;
		push(msg, i);
		if(to_flush(i))
		{
			//write data to disk
			flush_data(i); 
		}
	}

	void add_to_all(Message<PattT>& msg)
	{
		for(int i = 0; i < _num_miners; i++)
		{
			add_to(msg, i);
		}
	}

	void flush_all()
	{
		for(int i = 0; i < _num_miners; i++)
		{
			if(buffers[i].size() > 0)
			{
				flush_data(i);
			}
		}
	}

	void set_end(){
		Message<PattT> msg;
		msg.freq = -2;
		add_to_all(msg);
		flush_all();

		unique_lock<mutex> lock(mtx_batch);
		no_more_file = true;
		cond_batch.notify_one();
	}

private:
	void set_time(int dst)
	{
		start_times[dst] = get_current_time();
	}

	void push(Message<PattT>& msg, int dst)
	{
		if(buffers[dst].empty())
			set_time(dst);
		buffers[dst].push_back(msg);
	}

	bool to_flush(int dst) //need buffer flush?
	{
		if(buffers[dst].size() >= CAPACITY) return true;
		double current = get_current_time();
		if(current - start_times[dst] >= TIMEOUT) return true;
		return false;
	}

	void flush_data(int dst)
	{
		//cout<<"flush data"<<endl;
		ofbinstream &f = streams[dst];
		set_fname(fname, dst, batch_written[dst]);
		f.open(fname);

		MsgContainer &msgs = buffers[dst];
		for (int i = 0; i< msgs.size(); i++)
			 f << msgs[i];

		f.close();
		msgs.clear();

		//for parameter adjustment
		send_msg_batch ++;

		unique_lock<mutex> lock(mtx_batch);
		batch_written[dst] ++;
		numfile_written ++;
		cond_batch.notify_one();
		//cout<<"flush data"<<endl;
	}

};


template <class PattT>
class RecvBuf{
public:
	typedef vector<Message<PattT> > MsgContainer;

	MsgContainer buffer;
	int buf_pos;
	int buf_size;
	int count;

	int src;  //indicate where the last batch of messages from

	RecvBuf(){
		buf_pos = 0;
		buf_size = 0;
		count = 0;
	}

	void block_recv(int& from)// for master only
	{  
		block_recvData<MsgContainer>(buffer, from);
		buf_pos = 0;
		buf_size = buffer.size();
		count+=buf_size;
		//cout<<"total receive message "<<count<<endl;
	}


	void block_nextMsg(Message<PattT>& msg, int &from)// "from" indicate where the msg comes from 
	{ //should call has_next first
		if (buf_pos == buf_size) block_recv(src);
		msg = buffer[buf_pos ++];
		from = src;
	}

	bool unblock_recv() //for slaves only
	{
		if (unblock_recvData<MsgContainer>(buffer)){
			buf_pos = 0;
			buf_size = buffer.size();
			count+=buf_size;
			//cout<<"total receive message "<<count<<endl;
			return true;
		}
		else return false;
	}

	bool unblock_recv(int &from) //for master only
	{
		if (unblock_recvData<MsgContainer>(buffer, from)){
			buf_pos = 0;
			buf_size = buffer.size();
			count+=buf_size;
			//cout<<"total receive message "<<count<<endl;
			return true;
		}
		else return false;
	}

	bool unblock_nextMsg(Message<PattT>& msg){
		if(buf_pos == buf_size)
		{//reach buffer end
			bool has_msg = unblock_recv();
			if (has_msg)
			{
				msg = buffer[buf_pos++];
				return true;
			}
			else return false;
		}
		else{
			msg = buffer[buf_pos++];
			return true;
		}

	}

	bool unblock_nextMsg(Message<PattT>& msg, int &from){
		if(buf_pos == buf_size)
		{//reach buffer end
			bool has_msg = unblock_recv(src);
			if (has_msg)
			{
				msg = buffer[buf_pos++];
				from = src;
				return true;
			}
			else return false;
		}
		else{
			msg = buffer[buf_pos++];
			from = src;
			return true;
		}
	}

};

#endif
