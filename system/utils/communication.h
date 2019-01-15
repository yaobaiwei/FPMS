#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <mpi.h>
#include "serialization.h"
#include "global.h"

//============================================
//Allreduce
int all_sum(int my_copy)
{
    int tmp;
    MPI_Allreduce(&my_copy, &tmp, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    return tmp;
}

long long master_sum_LL(long long my_copy)
{
    long long tmp = 0;
    MPI_Reduce(&my_copy, &tmp, 1, MPI_LONG_LONG_INT, MPI_SUM, MASTER_RANK, MPI_COMM_WORLD);
    return tmp;
}

long long all_sum_LL(long long my_copy)
{
    long long tmp = 0;
    MPI_Allreduce(&my_copy, &tmp, 1, MPI_LONG_LONG_INT, MPI_SUM, MPI_COMM_WORLD);
    return tmp;
}

char all_bor(char my_copy)
{
    char tmp;
    MPI_Allreduce(&my_copy, &tmp, 1, MPI_BYTE, MPI_BOR, MPI_COMM_WORLD);
    return tmp;
}

//============================================
//scatter
template <class T>
void masterScatter(vector<T>& to_send)
{ //scatter
    int* sendcounts = new int[_num_workers];
    int recvcount;
    int* sendoffset = new int[_num_workers];

    obinstream m;
    int size = 0;
    for (int i = 0; i < _num_workers; i++) {
        if (i == _my_rank) {
            sendcounts[i] = 0;
        } else {
            m << to_send[i];
            sendcounts[i] = m.size() - size;
            size = m.size();
        }
    }

    MPI_Scatter(sendcounts, 1, MPI_INT, &recvcount, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    for (int i = 0; i < _num_workers; i++) {
        sendoffset[i] = (i == 0 ? 0 : sendoffset[i - 1] + sendcounts[i - 1]);
    }
    char* sendbuf = m.get_buf(); //obinstream will delete it
    char* recvbuf;

    MPI_Scatterv(sendbuf, sendcounts, sendoffset, MPI_CHAR, recvbuf, recvcount, MPI_CHAR, MASTER_RANK, MPI_COMM_WORLD);

    delete[] sendcounts;
    delete[] sendoffset;
}

template <class T>
void slaveScatter(T& to_get)
{ //scatter
    int* sendcounts;
    int recvcount;
    int* sendoffset;

    MPI_Scatter(sendcounts, 1, MPI_INT, &recvcount, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    char* sendbuf;
    char* recvbuf = new char[recvcount];

    MPI_Scatterv(sendbuf, sendcounts, sendoffset, MPI_CHAR, recvbuf, recvcount, MPI_CHAR, MASTER_RANK, MPI_COMM_WORLD);

    ibinstream um(recvbuf, recvcount);
    um >> to_get;
    delete recvbuf;
}

//================================================================
//gather
template <class T>
void masterGather(vector<T>& to_get)
{ //gather
    int sendcount = 0;
    int* recvcounts = new int[_num_workers];
    int* recvoffset = new int[_num_workers];

    MPI_Gather(&sendcount, 1, MPI_INT, recvcounts, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    for (int i = 0; i < _num_workers; i++) {
        recvoffset[i] = (i == 0 ? 0 : recvoffset[i - 1] + recvcounts[i - 1]);
    }

    char* sendbuf;
    int recv_tot = recvoffset[_num_workers - 1] + recvcounts[_num_workers - 1];
    char* recvbuf = new char[recv_tot];

    MPI_Gatherv(sendbuf, sendcount, MPI_CHAR, recvbuf, recvcounts, recvoffset, MPI_CHAR, MASTER_RANK, MPI_COMM_WORLD);

    ibinstream um(recvbuf, recv_tot);
    for (int i = 0; i < _num_workers; i++) {
        if (i == _my_rank)
            continue;
        um >> to_get[i];
    }
    delete[] recvcounts;
    delete[] recvoffset;
    delete[] recvbuf;
}

template <class T>
void slaveGather(T& to_send)
{ //gather
    int sendcount;
    int* recvcounts;
    int* recvoffset;

    obinstream m;
    m << to_send;
    sendcount = m.size();

    MPI_Gather(&sendcount, 1, MPI_INT, recvcounts, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    char* sendbuf = m.get_buf(); //obinstream will delete it
    char* recvbuf;

    MPI_Gatherv(sendbuf, sendcount, MPI_CHAR, recvbuf, recvcounts, recvoffset, MPI_CHAR, MASTER_RANK, MPI_COMM_WORLD);
}

//================================================================
//bcast
template <class T>
void masterBcast(T& to_send)
{ //broadcast
    obinstream m;
    m << to_send;
    int size = m.size();

    MPI_Bcast(&size, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    char* sendbuf = m.get_buf();
    MPI_Bcast(sendbuf, size, MPI_CHAR, MASTER_RANK, MPI_COMM_WORLD);
}

template <class T>
void slaveBcast(T& to_get)
{ //broadcast
    int size;

    MPI_Bcast(&size, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

    char* recvbuf = new char[size];
    MPI_Bcast(recvbuf, size, MPI_CHAR, MASTER_RANK, MPI_COMM_WORLD);

    ibinstream um(recvbuf, size);
    um >> to_get;

    delete[] recvbuf;
}

//============================================
#define COMM_CHANNEL_100 100//All are sent along channel 100, should not conflict with others
//char-level send/recv
void pregel_send(void* buf, int size, int dst)
{
    MPI_Send(buf, size, MPI_CHAR, dst, COMM_CHANNEL_100, MPI_COMM_WORLD);
}

void pregel_recv(void* buf, int size, int src)
{
    MPI_Recv(buf, size, MPI_CHAR, src, COMM_CHANNEL_100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

int pregel_recv(void* buf, int size) //any src
{
    MPI_Status status;
    MPI_Recv(buf, size, MPI_CHAR, MPI_ANY_SOURCE, COMM_CHANNEL_100, MPI_COMM_WORLD, &status);
    return status.MPI_SOURCE;
}

//------
//binstream-level send
void send_obinstream(obinstream& m, int dst)
{
    size_t size = m.size();
    pregel_send(&size, sizeof(size_t), dst);
    pregel_send(m.get_buf(), m.size(), dst);
}

ibinstream recv_ibinstream(int src, char* buf)
{
    size_t size;
    pregel_recv(&size, sizeof(size_t), src);
    buf = new char[size];
    pregel_recv(buf, size, src);
    return ibinstream(buf, size);
}

//------
//obj-level send/recv
template <class T>
void send_data(const T& data, int dst)
{
	obinstream m;
    m << data;
    send_obinstream(m, dst);
}

template <class T>
T recv_data(int src)
{
	size_t size;
	char* buf = new char[size];
	ibinstream um = recv_ibinstream(src, buf);
    T data;
    um >> data;
    delete buf;
    return data;
}

/*
ibinstream recv_ibinstream()//any src
{
    size_t size;
    int src = pregel_recv(&size, sizeof(size_t));
    char* buf = new char[size];
    pregel_recv(buf, size, src);
    return ibinstream(buf, size);
}
*/

ibinstream recv_ibinstream(int& src)//receive message from src(src is returned by pregel _recv)
{
    size_t size;
    src = pregel_recv(&size, sizeof(size_t));
    char* buf = new char[size];
    pregel_recv(buf, size, src);
    return ibinstream(buf, size);
}

template <class T>
void block_recvData(T &data) //used by master in alphabet_agg
{
    int src;
    ibinstream um = recv_ibinstream(src);
    um >> data;
}

template <class T>
void block_recvData(T &data, int& src)//receive message from src, src indicate where the msg comes from
{
    ibinstream um = recv_ibinstream(src);
    um >> data;
}

//-------add by Zitao Li for prefixspan--------
bool has_msg() //for unblock recv
{
    MPI_Status status;
    int flag;
    MPI_Iprobe(MPI_ANY_SOURCE, COMM_CHANNEL_100, MPI_COMM_WORLD, &flag, &status);
    return flag;
}

template <class T>
bool unblock_recvData(T &data)
{
    if (has_msg())
    {
        int src;
        ibinstream um = recv_ibinstream(src);
        um >> data;
        return true;
    }
    else return false;
}

template <class T>
bool unblock_recvData(T &data, int &src)
{
    if (has_msg())
    {
        ibinstream um = recv_ibinstream(src);
        um >> data;
        return true;
    }
    else return false;
}


#endif
