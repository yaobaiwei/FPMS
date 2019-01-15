#ifndef PATTERN_H
#define PATTERN_H

#include "../utils/ioser.h"


template<class DeltaT>
class Pattern: public vector<DeltaT>
{
public:
	typedef DeltaT DeltaType;
	typedef typename Pattern::iterator PIter;

	PIter next;

	virtual void grow(const DeltaType & d, Pattern & new_patt) = 0;

	inline void init_start()
	{
		next = this->begin();
	}

	inline DeltaType next_delta()
	{
		PIter cur = next;
		next ++;
		return *cur;
	}
	
	//UDF
	virtual inline void print() = 0;

	
	Pattern & operator=(const Pattern &p2)
	{
		this->clear();
		for(int i = 0; i < p2.size(); i++)
			this->push_back(p2.at(i));
		return *this;
	} 
	

};


#endif