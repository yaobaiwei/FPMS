#ifndef PROJTRAN_H
#define PROJTRAN_H

#include "Pattern.h"

#define NULLRef(type) (*((type*)(NULL))) //used by gspan

template <class TranT, class PattT>
class ProjTran{

public:
	typedef typename TranT::ItemType ItemT;
	typedef TranT TranType;//for pass to higher class using ProjTran
	typedef ItemT ItemType; //for pass to higher class using ProjTran

	typedef typename PattT::DeltaType DeltaT;
	typedef unordered_map<DeltaT, ProjTran*> ProjT_map;
	typedef DeltaT DeltaType;//for pass to higher class using ProjTran
	
	typedef PattT PattType;

	int tran_id;
	TranT &transaction;

	ProjTran():transaction(NULLRef(TranT)){}

   	ProjTran(TranT & t, int id):transaction(t)
   	{
   		tran_id = id;
   	}

   	//virtual void project(ProjT_map & tmp) = 0;

};

#endif