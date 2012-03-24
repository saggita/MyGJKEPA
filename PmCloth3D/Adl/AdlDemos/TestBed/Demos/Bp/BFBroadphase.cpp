#include <Demos/Bp/BFBroadphase.h>


int BFBroadphase::getPair(const Aabb* aabbs, int nAabbs, Pair32* pairsOut, int capacity)
{
	return getPairImpl( aabbs, nAabbs, pairsOut, capacity );
}

int BFBroadphase::getPair(const AabbUint* aabbs, int nAabbs, Pair32* pairsOut, int capacity)
{
	return getPairImpl( aabbs, nAabbs, pairsOut, capacity );
}

template<typename BOUNDINGVOLUME>
int BFBroadphase::getPairImpl(const BOUNDINGVOLUME* aabbs, int nAabbs, Pair32* pairsOut, int capacity)
{
	int nPairs = 0;
	for(int i=0; i<nAabbs; i++)
	{
		const BOUNDINGVOLUME& iAabb = aabbs[i];
		for(int j=i+1; j<nAabbs; j++)
		{
			const BOUNDINGVOLUME& jAabb = aabbs[j];

			if( iAabb.overlaps( jAabb ) )
			{
				pairsOut[ nPairs++ ] = Pair32( i, j );

				if( nPairs == capacity ) return nPairs;
			}
		}
	}

	return nPairs;
}
