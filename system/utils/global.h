#ifndef GLOBAL_H
#define GLOBAL_H

#include <mpi.h>
#include <stddef.h>
#include <string>
#include <vector>
#include <ext/hash_set>
#include <unordered_set>
#include <ext/hash_map>
#include <unordered_map>
#include <map>
#include <string.h>
#include <mutex>
#include <condition_variable>
#include <stddef.h>
#include <sys/stat.h>
#include <assert.h>
#include <utility>  
#include <iterator> //advance()
#define hash_map __gnu_cxx::hash_map
#define hash_set __gnu_cxx::hash_set
using namespace std;

//============================
///worker info
#define MASTER_RANK 0

#define ItemID short //internal ID for an item

int _my_rank;
int _num_workers;
int _num_miners;

double freq_threshold;

//global count for master
//local count for slaves
long long total_count;
double req_count;

int elements_size;
vector<long long> _num_trans;
//----------------------

//parameters for message buffer(s)
#define REQUEST -1
#define NA -1
#define JOB_END -2
#define INFREQ -3
#define MIN_FRONTIER -4
#define MIN_FRONTIER_TO_END -5
//double TIMEOUT; //define after how long the buffer will be flushed at most
//int CAPACITY; //define the send buffers capacity

#define TIMEOUT 0.1
#define CAPACITY 200

string SENDER_FILE = "send"; //prefix of the message files on master side
string SENDER_DIR  = "/tmp/prefixspan/"; //place to store the messages files

vector<int> batch_written; //record how many batches the main thread has generated
int numfile_written; //record the total number of files 
mutex mtx_batch; //a mutex to protect the conditional variable
condition_variable cond_batch; // a conditional variable used by main thread to notify
                                // another thread
bool no_more_file;  //used by main thread to indicate that no more file will be generated
//------------------------------

//---------------------------------------------
#define SLAVE_POS2ID(x) ((x)+1)
#define SLAVE_ID2POS(x) ((x)-1)
//---------------------------------------------

/*/-------PDB-------------------------------------
typedef vector<ItemID> Transaction;
typedef vector<Transaction> TransactionContainer;
typedef int TransactionID;

TransactionContainer transactions;
*///-----------------------------------------------

//--------For adjusting parameters---------------
long max_num_MasterNode, num_MasterNode;
long max_num_SlaveNode, num_SlaveNode;
long total_request;
long send_msg_batch;
//-----------------------------------------------

inline int get_worker_id()
{
    return _my_rank;
}
inline int get_num_workers()
{
    return _num_workers;
}

void init_workers()
{
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &_num_workers);
    MPI_Comm_rank(MPI_COMM_WORLD, &_my_rank);
    _num_miners = _num_workers - 1;
}

void worker_finalize()
{
    MPI_Finalize();
}

void worker_barrier()
{
    MPI_Barrier(MPI_COMM_WORLD);
}

void worker_abort()
{
    MPI_Abort(MPI_COMM_WORLD, 1);
}

void _mkdir(const char *dir) {//taken from: http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if(tmp[len - 1] == '/') tmp[len - 1] = '\0';
    for(p = tmp + 1; *p; p++)
        if(*p == '/') {
                *p = 0;
                mkdir(tmp, S_IRWXU);
                *p = '/';
        }
    mkdir(tmp, S_IRWXU);
}

//------------------------
// worker parameters

struct WorkerParams {
    string input_path; //HDFS
    string output_path; //master's local FS
    double threshold;
};

#endif
