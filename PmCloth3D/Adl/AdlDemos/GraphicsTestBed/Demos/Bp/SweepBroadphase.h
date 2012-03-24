#pragma once


#include <Demos/Bp/BroadphaseBase.h>


class SweepBroadphase : public BroadphaseBase
{
	public:
		virtual int getPair(const Aabb* aabbs, int nAabbs, Pair32* pairsOut, int capacity);
		virtual int getPair(const AabbUint* aabbs, int nAabbs, Pair32* pairsOut, int capacity);
};


