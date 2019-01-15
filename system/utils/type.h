#ifndef TYPE_H_
#define TYPE_H_

#include "serialization.h"
#include "global.h"
#include <algorithm>
#include "string.h"

//equivalent to boost::hash_combine
void hash_combine(size_t& seed, int v)
{
    seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

//============= int pair =============
struct intpair {
    int v1;
    int v2;

    intpair()
    {
    }

    intpair(int v1, int v2)
    {
        this->v1 = v1;
        this->v2 = v2;
    }

    void set(int v1, int v2)
    {
        this->v1 = v1;
        this->v2 = v2;
    }

    inline bool operator<(const intpair& rhs) const
    {
        return (v1 < rhs.v1) || ((v1 == rhs.v1) && (v2 < rhs.v2));
    }

    inline bool operator>(const intpair& rhs) const
    {
        return (v1 > rhs.v1) || ((v1 == rhs.v1) && (v2 > rhs.v2));
    }

    inline bool operator==(const intpair& rhs) const
    {
        return (v1 == rhs.v1) && (v2 == rhs.v2);
    }

    inline bool operator!=(const intpair& rhs) const
    {
        return (v1 != rhs.v1) || (v2 != rhs.v2);
    }

    int hash()
    {
        size_t seed = 0;
        hash_combine(seed, v1);
        hash_combine(seed, v2);
        return seed % ((unsigned int)_num_workers);
    }
};

ibinstream& operator<<(ibinstream& m, const intpair& v)
{
    m << v.v1;
    m << v.v2;
    return m;
}

obinstream& operator>>(obinstream& m, intpair& v)
{
    m >> v.v1;
    m >> v.v2;
    return m;
}

class IntPairHash {
public:
    inline int operator()(intpair key)
    {
        return key.hash();
    }
};

namespace __gnu_cxx {
template <>
struct hash<intpair> {
    size_t operator()(intpair pair) const
    {
        size_t seed = 0;
        hash_combine(seed, pair.v1);
        hash_combine(seed, pair.v2);
        return seed;
    }
};
}

//============= int triplet =============
struct inttriplet {
    int v1;
    int v2;
    int v3;

    inttriplet()
    {
    }

    inttriplet(int v1, int v2, int v3)
    {
        this->v1 = v1;
        this->v2 = v2;
        this->v3 = v3;
    }

    void set(int v1, int v2, int v3)
    {
        this->v1 = v1;
        this->v2 = v2;
        this->v3 = v3;
    }

    inline bool operator<(const inttriplet& rhs) const
    {
        return (v1 < rhs.v1) || ((v1 == rhs.v1) && (v2 < rhs.v2)) || ((v1 == rhs.v1) && (v2 == rhs.v2) && (v3 < rhs.v3));
    }

    inline bool operator>(const inttriplet& rhs) const
    {
        return (v1 > rhs.v1) || ((v1 == rhs.v1) && (v2 > rhs.v2)) || ((v1 == rhs.v1) && (v2 == rhs.v2) && (v3 > rhs.v3));
    }

    inline bool operator==(const inttriplet& rhs) const
    {
        return (v1 == rhs.v1) && (v2 == rhs.v2) && (v3 == rhs.v3);
    }

    inline bool operator!=(const inttriplet& rhs) const
    {
        return (v1 != rhs.v1) || (v2 != rhs.v2) || (v3 != rhs.v3);
    }

    int hash()
    {
        size_t seed = 0;
        hash_combine(seed, v1);
        hash_combine(seed, v2);
        hash_combine(seed, v3);
        return seed % ((unsigned int)_num_workers);
    }
};

ibinstream& operator<<(ibinstream& m, const inttriplet& v)
{
    m << v.v1;
    m << v.v2;
    m << v.v3;
    return m;
}

obinstream& operator>>(obinstream& m, inttriplet& v)
{
    m >> v.v1;
    m >> v.v2;
    m >> v.v3;
    return m;
}

class IntTripletHash {
public:
    inline int operator()(inttriplet key)
    {
        return key.hash();
    }
};

namespace __gnu_cxx {
template <>
struct hash<inttriplet> {
    size_t operator()(inttriplet triplet) const
    {
        size_t seed = 0;
        hash_combine(seed, triplet.v1);
        hash_combine(seed, triplet.v2);
        hash_combine(seed, triplet.v3);
        return seed;
    }
};
}

//============= vertex-worker pair =============
struct vwpair {
    VertexID vid;
    int wid;

    vwpair()
    {
    }

    vwpair(int vid, int wid)
    {
        this->vid = vid;
        this->wid = wid;
    }

    void set(int vid, int wid)
    {
        this->vid = vid;
        this->wid = wid;
    }

    inline bool operator<(const vwpair& rhs) const
    {
        return vid < rhs.vid;
    }

    inline bool operator==(const vwpair& rhs) const
    {
        return vid == rhs.vid;
    }

    inline bool operator!=(const vwpair& rhs) const
    {
        return vid != rhs.vid;
    }

    int hash()
    {
        return wid; //the only difference from intpair
    }
};

ibinstream& operator<<(ibinstream& m, const vwpair& v)
{
    m << v.vid;
    m << v.wid;
    return m;
}

obinstream& operator>>(obinstream& m, vwpair& v)
{
    m >> v.vid;
    m >> v.wid;
    return m;
}

class VWPairHash {
public:
    inline int operator()(vwpair key)
    {
        return key.hash();
    }
};

namespace __gnu_cxx {
template <>
struct hash<vwpair> {
    size_t operator()(vwpair pair) const
    {
        return pair.vid;
    }
};
}

//============= to allow string to be ID =============

namespace __gnu_cxx {
	template <>
	struct hash<string> {
		size_t operator()(string str) const
		{
			return __stl_hash_string(str.c_str());
		}
	};
}

class StringHash {
	public:
		inline int operator()(string str)
		{
			return __gnu_cxx::__stl_hash_string(str.c_str()) % ((unsigned int)_num_workers);
		}
};

//============= space efficient string-set =============
struct string_set{
    char num_bytes;
    int num_strs;
    void* position;
    char* buf;

    string_set()
    {
    	num_bytes=0;
    	num_strs=0;
    	position=NULL;
    	buf=NULL;
    }

    ~string_set()
	{
    	if(num_bytes!=0)
		{
			delete buf;
			if(num_bytes==1) delete (unsigned char*)position;
			else if(num_bytes==2) delete (unsigned short*)position;
			else delete (unsigned int*)position;
		}
	}

    void set(vector<string> & strs)
	{
    	if(strs.size()==0) return;
    	std::set<string> str_set;
    	for(int i=0; i<strs.size(); i++)
		{
    		str_set.insert(strs[i]);
		}
		num_strs=str_set.size();
		int nbuf=0;
		for(std::set<string>::iterator it=str_set.begin(); it!=str_set.end(); it++)
		{
			nbuf+=it->length()+1;
		}
		buf=new char[nbuf];
		int pos=0;
		int i=0;
		if(nbuf<256){
			num_bytes=1;
			position=new unsigned char[num_strs];
			unsigned char * pos_array = (unsigned char *)position;
			for(std::set<string>::iterator it=str_set.begin(); it!=str_set.end(); it++)
			{
				int len=it->length();
				pos_array[i]=(unsigned char)pos;
				strcpy(buf+pos, it->c_str());
				pos+=len+1;
				i++;
			}
		}
		else if(nbuf<65536){
			num_bytes=2;
			position=new unsigned short[num_strs];
			unsigned short * pos_array = (unsigned short *)position;
			for(std::set<string>::iterator it=str_set.begin(); it!=str_set.end(); it++)
			{
				int len=it->length();
				pos_array[i]=(unsigned short)pos;
				strcpy(buf+pos, it->c_str());
				pos+=len+1;
				i++;
			}
		}
		else{
			num_bytes=4;
			position=new unsigned int[num_strs];
			unsigned int * pos_array = (unsigned int *)position;
			for(std::set<string>::iterator it=str_set.begin(); it!=str_set.end(); it++)
			{
				int len=it->length();
				pos_array[i]=(unsigned int)pos;
				strcpy(buf+pos, it->c_str());
				pos+=len+1;
				i++;
			}
		}
	}

    //need to make sure "strs" has no redundancy
    void set_nocheck(vector<string> & strs)
    {
    	if(strs.size()==0) return;
    	sort(strs.begin(), strs.end());
    	num_strs=strs.size();
    	int nbuf=0;
    	for(int i=0; i<num_strs; i++)
    	{
    		nbuf+=strs[i].length()+1;
    	}
    	buf=new char[nbuf];
    	int pos=0;
    	if(nbuf<256){
    		num_bytes=1;
    		position=new unsigned char[num_strs];
    		unsigned char * pos_array = (unsigned char *)position;
    		for(int i=0; i<num_strs; i++)
			{
				int len=strs[i].length();
				pos_array[i]=(unsigned char)pos;
				strcpy(buf+pos, strs[i].c_str());
				pos+=len+1;
			}
    	}
    	else if(nbuf<65536){
    		num_bytes=2;
    		position=new unsigned short[num_strs];
    		unsigned short * pos_array = (unsigned short *)position;
    		for(int i=0; i<num_strs; i++)
			{
				int len=strs[i].length();
				pos_array[i]=(unsigned short)pos;
				strcpy(buf+pos, strs[i].c_str());
				pos+=len+1;
			}
    	}
    	else{
    		num_bytes=4;
    		position=new unsigned int[num_strs];
    		unsigned int * pos_array = (unsigned int *)position;
    		for(int i=0; i<num_strs; i++)
			{
				int len=strs[i].length();
				pos_array[i]=(unsigned int)pos;
				strcpy(buf+pos, strs[i].c_str());
				pos+=len+1;
			}
    	}
    }

    char* get_string(int pos) const
    {
    	if(num_bytes==0) return NULL;
    	else if(num_bytes==1)
    	{
    		unsigned char * pos_array = (unsigned char *)position;
    		return buf+pos_array[pos];
    	}
    	else if(num_bytes==2)
    	{
			unsigned short * pos_array = (unsigned short *)position;
			return buf+pos_array[pos];
		}
    	else
    	{
			unsigned int * pos_array = (unsigned int *)position;
			return buf+pos_array[pos];
		}
    }

    bool contains(const char* str)//binary search
    {
    	if(num_bytes==0) return false;
    	int low = 0;
    	int high = num_strs-1;
    	while(low <= high)
    	{
    		int mid = (low+high)/2;
    		char* mid_str = get_string(mid);
    		int cmp=strcmp(mid_str, str);
    		if(cmp < 0)
                low = mid+1;
            else if(cmp > 0)
            	high = mid-1;
            else
                return true;
        }
        return false;
    }
};

ibinstream& operator<<(ibinstream& m, const string_set& v)
{
	m << v.num_strs;
    for(int i=0; i<v.num_strs; i++)
    {
    	char* str = v.get_string(i);
    	size_t len=strlen(str);
    	m << len;
    	for(int j=0; j<len; j++) m << str[j];
    }
    return m;
}

obinstream& operator>>(obinstream& m, string_set& v)
{
	int nstr;
	m >> nstr;
	vector<string> vec(nstr);
	for(int i=0; i<nstr; i++){
		m >> vec[i];
	}
	v.set_nocheck(vec);
    return m;
}

#endif /* TYPE_H_ */
