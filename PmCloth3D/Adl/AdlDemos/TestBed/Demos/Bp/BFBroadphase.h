#ifndef BF_BROADPHASE_H
#define BF_BROADPHASE_H

#include <Demos/Bp/BroadphaseBase.h>


class BFBroadphase : public BroadphaseBase
{
	public:
		template<typename BOUNDINGVOLUME>
		int getPairImpl(const BOUNDINGVOLUME* aabbs, int nAabbs, Pair32* pairsOut, int capacity);

		virtual int getPair(const Aabb* aabbs, int nAabbs, Pair32* pairsOut, int capacity);
		virtual int getPair(const AabbUint* aabbs, int nAabbs, Pair32* pairsOut, int capacity);
};


#endif

