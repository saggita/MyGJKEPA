#include <Demos/Bp/SweepBroadphase.h>
#include <AdlPrimitives/Sort/RadixSort.h>


int SweepBroadphase::getPair(const Aabb* aabbs, int nAabbs, Pair32* pairsOut, int capacity)
{

	return 0;
}

int SweepBroadphase::getPair(const AabbUint* aabbs, int nAabbs, Pair32* pairsOut, int capacity)
{
	DeviceUtils::Config cfg;
	Device* hostDevice = DeviceUtils::allocate( TYPE_HOST, cfg );

	Pair32* pairStart = pairsOut;
	{
		RadixSort<TYPE_HOST>::Data* s = RadixSort<TYPE_HOST>::allocate( hostDevice, nAabbs );

		HostBuffer<SortData> sortData( hostDevice, nAabbs );
		for(int i=0; i<nAabbs; i++)
		{
			sortData[i].m_key = aabbs[i].m_min[0];
			sortData[i].m_value = i;
		}

		RadixSort<TYPE_HOST>::execute( s, sortData, nAabbs );


		for(int i=0; i<nAabbs; i++)
		{
			int iIdx = sortData[i].m_value;
			const AabbUint& iAabb = aabbs[iIdx];
			for(int j=i+1; j<nAabbs; j++)
			{
				int jIdx = sortData[j].m_value;
				const AabbUint& jAabb = aabbs[jIdx];

				if( iAabb.m_max[0] < jAabb.m_min[0] ) break;

				if( iAabb.overlaps( jAabb ) )
				{
					(*pairsOut++) = Pair32( iIdx, jIdx );
				}
			}
		}

		RadixSort<TYPE_HOST>::deallocate( s );
	}
	
	DeviceUtils::deallocate( hostDevice );

	return pairsOut - pairStart;
}

