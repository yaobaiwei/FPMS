//Acknowledgements: the operator overloading is implemented based on pregel-mpi (https://code.google.com/p/pregel-mpi/) by Chuntao Hong.

#ifndef IOSER_H
#define IOSER_H

#include <stdio.h> //for FILE pointer
#include <string.h> //for memcpy
#include <sys/stat.h> //for file size
#include "serialization.h"

#include <vector>
#include <set>
#include <string>
#include <map>

#include "global.h"

using namespace std;

#define STREAM_MEMBUF_SIZE  10//10 bytes

//-------------------------------------

class ofbinstream {

private:
    char* membuf;
    int bufpos;
    size_t totpos;
    FILE * file;

public:

    ofbinstream()//empty
	{
		file = NULL;
		bufpos = 0;
		totpos = 0;
		membuf = new char[STREAM_MEMBUF_SIZE];
	}

    ofbinstream(const char* path)
    {
    	file = fopen(path, "wb");
    	bufpos = 0;
    	totpos = 0;
    	membuf = new char[STREAM_MEMBUF_SIZE];
    }

    inline void bufflush()
    {
    	fwrite(membuf, 1, bufpos, file);
    }

    ~ofbinstream()
	{
    	delete membuf;
    	if(file == NULL) return; //already closed
    	if(bufpos > 0) bufflush();
    	fclose(file);
	}

    inline size_t size()
    {
        return totpos;
    }

    void raw_byte(char c)
    {
    	if(bufpos == STREAM_MEMBUF_SIZE)
    	{
    		bufflush();
    		bufpos = 0;
    	}
    	membuf[bufpos] = c;
    	bufpos++;
        totpos++;
    }

    void raw_bytes(const void* ptr, int size)
    {
    	totpos += size;
    	int gap = STREAM_MEMBUF_SIZE - bufpos;
    	char * cptr = (char *)ptr;
    	if(gap < size)
    	{
    		memcpy(membuf + bufpos, cptr, gap);
    		bufpos = STREAM_MEMBUF_SIZE; //useful for correct exec of bufflush()
    		bufflush();
    		size -= gap;
    		cptr += gap;
    		while(size > STREAM_MEMBUF_SIZE)
    		{
    			memcpy(membuf, cptr, STREAM_MEMBUF_SIZE);
    			bufflush();
    			size -= STREAM_MEMBUF_SIZE;
    			cptr += STREAM_MEMBUF_SIZE;
    		}
    		memcpy(membuf, cptr, size);
    		bufpos = size;
    	}
    	else
    	{
    		memcpy(membuf + bufpos, ptr, size);
    		bufpos += size;
    	}
    }

    //below is for object reuse

    void close() //also for flushing
    {
    	if(file == NULL) return; //already closed
    	if(bufpos > 0) bufflush();
    	fclose(file);
    	file = NULL; //set status to closed
    }

    void open(const char* path) //it does not check whether you closed previous file
    {
    	file = fopen(path,"wb");
		bufpos = 0;
		totpos = 0;
    }

    bool is_open()
    {
    	return file != NULL;
    }

};

//make sure mm only contains one object (mm should be cleared before serializing an object)
ofbinstream & operator<<(ofbinstream & m, obinstream mm)
{
    m.raw_bytes(mm.get_buf(), mm.size());
    return m;
}

ofbinstream & operator<<(ofbinstream & m, size_t i)
{
    m.raw_bytes(&i, sizeof(size_t));
    return m;
}

ofbinstream & operator<<(ofbinstream & m, bool i)
{
    m.raw_bytes(&i, sizeof(bool));
    return m;
}

ofbinstream & operator<<(ofbinstream & m, int i)
{
    m.raw_bytes(&i, sizeof(int));
    return m;
}

ofbinstream & operator<<(ofbinstream & m, double i)
{
    m.raw_bytes(&i, sizeof(double));
    return m;
}

ofbinstream & operator<<(ofbinstream & m, char c)
{
    m.raw_byte(c);
    return m;
}

//--------------add for frequent pattern mining------
ofbinstream& operator<<(ofbinstream &m, long long i)
{
    m.raw_bytes(&i, sizeof(long long));
    return m;
}
ofbinstream& operator<<(ofbinstream &m, short i)
{
    m.raw_bytes(&i, sizeof(short));
    return m;
}
//-----------------------------------------------------

template <class T>
ofbinstream & operator<<(ofbinstream & m, const T* p)
{
    return m << *p;
}

template <class T>
ofbinstream & operator<<(ofbinstream & m, const vector<T>& v)
{
    m << v.size();
    for (typename vector<T>::const_iterator it = v.begin(); it != v.end(); ++it) {
        m << *it;
    }
    return m;
}

template <>
ofbinstream & operator<<(ofbinstream & m, const vector<int> & v)
{
    m << v.size();
    m.raw_bytes(&v[0], v.size() * sizeof(int));
    return m;
}

template <>
ofbinstream & operator<<(ofbinstream & m, const vector<double> & v)
{
    m << v.size();
    m.raw_bytes(&v[0], v.size() * sizeof(double));
    return m;
}

template <class T>
ofbinstream & operator<<(ofbinstream & m, const set<T> & v)
{
    m << v.size();
    for(typename set<T>::const_iterator it = v.begin(); it != v.end(); ++it) {
        m << *it;
    }
    return m;
}

ofbinstream & operator<<(ofbinstream & m, const string & str)
{
    m << str.length();
    m.raw_bytes(str.c_str(), str.length());
    return m;
}

template <class KeyT, class ValT>
ofbinstream & operator<<(ofbinstream & m, const map<KeyT, ValT> & v)
{
    m << v.size();
    for (typename map<KeyT, ValT>::const_iterator it = v.begin(); it != v.end(); ++it) {
        m << it->first;
        m << it->second;
    }
    return m;
}

template <class KeyT, class ValT>
ofbinstream & operator<<(ofbinstream & m, const hash_map<KeyT, ValT> & v)
{
    m << v.size();
    for (typename hash_map<KeyT, ValT>::const_iterator it = v.begin(); it != v.end(); ++it) {
        m << it->first;
        m << it->second;
    }
    return m;
}

template <class T>
ofbinstream & operator<<(ofbinstream & m, const hash_set<T> & v)
{
    m << v.size();
    for (typename hash_set<T>::const_iterator it = v.begin(); it != v.end(); ++it) {
        m << *it;
    }
    return m;
}

//-------------------------------------

class ifbinstream {

private:
	char* membuf;
	int bufpos;
	int bufsize; //membuf may not be full (e.g. last batch)
	size_t totpos;
	size_t filesize;
	FILE * file;

public:
	inline void fill()
	{
		bufsize = fread(membuf, 1, STREAM_MEMBUF_SIZE, file);
		bufpos = 0;
	}

	ifbinstream()
	{
		membuf = new char[STREAM_MEMBUF_SIZE];
		file = NULL; //set status to closed
	}

	ifbinstream(const char* path)
	{
		membuf = new char[STREAM_MEMBUF_SIZE];
		file = fopen(path, "rb");
		//get file size
		filesize = -1;
		struct stat statbuff;
		if(stat(path, &statbuff) == 0) filesize = statbuff.st_size;
		//get first batch
		fill();
		totpos = 0;
	}

	bool open(const char* path) //return whether the file exists
	{
		file = fopen(path, "rb");
		if(file == NULL) return false;
		//get file size
		filesize = -1;
		struct stat statbuff;
		if(stat(path, &statbuff) == 0) filesize = statbuff.st_size;
		//get first batch
		fill();
		totpos = 0;
		return true;
	}

	inline size_t size()
	{
		return filesize;
	}

	inline bool eof()
	{
		return totpos >= filesize;
	}

    ~ifbinstream()
    {
    	delete membuf;
    	if(file == NULL) return; //already closed
		fclose(file);
    }

    char raw_byte()
    {
    	totpos++;
    	if(bufpos == bufsize) fill();
        return membuf[bufpos++];
    }

    void* raw_bytes(unsigned int n_bytes)
    {
    	totpos += n_bytes;
    	int gap = bufsize - bufpos;
    	if(gap >= n_bytes)
    	{
    		char* ret = membuf + bufpos;
    		bufpos += n_bytes;
			return ret;
    	}
    	else
    	{
    		//copy the last gap-batch to head of membuf
    		//!!! require that STREAM_MEMBUF_SIZE >= n_bytes !!!
    		memcpy(membuf, membuf + bufpos, gap);
    		//gap-shifted refill
    		bufsize = gap + fread(membuf + gap, 1, STREAM_MEMBUF_SIZE - gap, file);
    		bufpos = n_bytes;
    		return membuf;
    	}
    }

    void close()
	{
    	if(file == NULL) return; //already closed
		fclose(file);
		file = NULL; //set status to closed
	}

    //=============== add skip function ===============
    void skip(int num_bytes)
    {
    	totpos += num_bytes;
    	if(totpos >= filesize) return; //eof
    	bufpos += num_bytes; //done if bufpos < bufsize
    	if(bufpos >= bufsize)
    	{
    		fseek(file, bufpos - bufsize, SEEK_CUR);
    		fill();
    	}
    }
};

ifbinstream & operator>>(ifbinstream & m, size_t & i)
{
    i = *(size_t*)m.raw_bytes(sizeof(size_t));
    return m;
}

ifbinstream & operator>>(ifbinstream & m, bool & i)
{
    i = *(bool*)m.raw_bytes(sizeof(bool));
    return m;
}

ifbinstream & operator>>(ifbinstream & m, int & i)
{
    i = *(int*)m.raw_bytes(sizeof(int));
    return m;
}

ifbinstream & operator>>(ifbinstream & m, double & i)
{
    i = *(double*)m.raw_bytes(sizeof(double));
    return m;
}

ifbinstream & operator>>(ifbinstream & m, char & c)
{
    c = m.raw_byte();
    return m;
}

//---------------add for frequent pattern mining----
ifbinstream & operator>>(ifbinstream & m, long long & i)
{
    i = *(long long*)m.raw_bytes(sizeof(long long));
    return m;
}

ifbinstream & operator>>(ifbinstream & m, short & i)
{
    i = *(short*)m.raw_bytes(sizeof(short));
    return m;
}
//------------------------------------------

template <class T>
ifbinstream & operator>>(ifbinstream & m, T* & p)
{
    p = new T;
    return m >> (*p);
}

template <class T>
ifbinstream & operator>>(ifbinstream & m, vector<T> & v)
{
    size_t size;
    m >> size;
    v.resize(size);
    for (typename vector<T>::iterator it = v.begin(); it != v.end(); ++it) {
        m >> *it;
    }
    return m;
}

template <>
ifbinstream & operator>>(ifbinstream & m, vector<int> & v)
{
    size_t size;
    m >> size;
    vector<int>::iterator it = v.begin();
    int len = STREAM_MEMBUF_SIZE / sizeof(int);
    int bytes = len * sizeof(int);
    while(size > len)
    {
        int* data = (int*)m.raw_bytes(bytes);
        v.insert(it, data, data + len);
        it = v.end();
        size -= len;
    }
    int* data = (int*)m.raw_bytes(sizeof(int) * size);
    v.insert(it, data, data + size);
    return m;
}

template <>
ifbinstream & operator>>(ifbinstream & m, vector<double> & v)
{
    size_t size;
    m >> size;
    vector<double>::iterator it = v.begin();
	int len = STREAM_MEMBUF_SIZE / sizeof(double);
	int bytes = len * sizeof(double);
    while(size > len)
    {
        double* data = (double*)m.raw_bytes(bytes);
        v.insert(it, data, data + len);
        it = v.end();
        size -= len;
    }
    double* data = (double*)m.raw_bytes(sizeof(double) * size);
    v.insert(it, data, data + size);
    return m;
}

template <class T>
ifbinstream & operator>>(ifbinstream & m, set<T> & v)
{
    size_t size;
    m >> size;
    for (size_t i = 0; i < size; i++) {
        T tmp;
        m >> tmp;
        v.insert(v.end(), tmp);
    }
    return m;
}

ifbinstream & operator>>(ifbinstream & m, string & str)
{
    size_t length;
    m >> length;
    str.clear();

    while(length > STREAM_MEMBUF_SIZE)
    {
        char* data = (char*)m.raw_bytes(STREAM_MEMBUF_SIZE); //raw_bytes cannot accept input > STREAM_MEMBUF_SIZE
        str.append(data, STREAM_MEMBUF_SIZE);
        length -= STREAM_MEMBUF_SIZE;
    }
    char* data = (char*)m.raw_bytes(length);
    str.append(data, length);

    return m;
}

template <class KeyT, class ValT>
ifbinstream & operator>>(ifbinstream & m, map<KeyT, ValT> & v)
{
    size_t size;
    m >> size;
    for (int i = 0; i < size; i++) {
        KeyT key;
        m >> key;
        m >> v[key];
    }
    return m;
}

template <class KeyT, class ValT>
ifbinstream & operator>>(ifbinstream & m, hash_map<KeyT, ValT> & v)
{
    size_t size;
    m >> size;
    for (int i = 0; i < size; i++) {
        KeyT key;
        m >> key;
        m >> v[key];
    }
    return m;
}

template <class T>
ifbinstream & operator>>(ifbinstream & m, hash_set<T> & v)
{
    size_t size;
    m >> size;
    for (int i = 0; i < size; i++) {
        T key;
        m >> key;
        v.insert(key);
    }
    return m;
}

#endif
