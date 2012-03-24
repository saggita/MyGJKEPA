#ifndef BROADPHASE_BASE_H
#define BROADPHASE_BASE_H

#include <AdlPrimitives/Math/Math.h>
#include <Common/Geometry/Aabb.h>


struct AabbUint
{
	u32 m_max[4];
	u32 m_min[4];


	__inline
	bool overlaps( const AabbUint& aabb ) const;

	__inline
	static bool overlaps( const AabbUint& a, const AabbUint& b );
};

bool AabbUint::overlaps( const AabbUint& in ) const
{
	return overlaps( *this, in );
}

bool AabbUint::overlaps( const AabbUint& a, const AabbUint& b )
{
	if( a.m_max[0] < b.m_min[0] || a.m_min[0] > b.m_max[0] ) return false;
	if( a.m_max[1] < b.m_min[1] || a.m_min[1] > b.m_max[1] ) return false;
	if( a.m_max[2] < b.m_min[2] || a.m_min[2] > b.m_max[2] ) return false;

	return true;
}

__inline
void convertToAabbUint(const Aabb* aabbIn, AabbUint* aabbOut, int n, const Aabb& space)
{
	float4 extent = space.getExtent();
	float4 c = make_float4(1.f/extent.x, 1.f/extent.y, 1.f/extent.z );

	for(int i=0; i<n; i++)
	{
		const Aabb& aabb = aabbIn[i];
		float4 ma, mi;
		ma = (aabb.m_max-space.m_min)*c;
		mi = (aabb.m_min-space.m_min)*c;

		ADLASSERT( ma.x >= 0 );
		ADLASSERT( ma.y >= 0 );
		ADLASSERT( ma.z >= 0 );

		ADLASSERT( mi.x >= 0 );
		ADLASSERT( mi.y >= 0 );
		ADLASSERT( mi.z >= 0 );

		aabbOut[i].m_max[0] = u32(ma.x*0xffff)+1;
		aabbOut[i].m_max[1] = u32(ma.y*0xffff)+1;
		aabbOut[i].m_max[2] = u32(ma.z*0xffff)+1;

		aabbOut[i].m_min[0] = u32(mi.x*0xffff);
		aabbOut[i].m_min[1] = u32(mi.y*0xffff);
		aabbOut[i].m_min[2] = u32(mi.z*0xffff);

		int a=0;
		a++;
	}
}

class BroadphaseBase
{
	public:
		virtual int getPair(const Aabb* aabbs, int nAabbs, Pair32* pairsOut, int capacity) = 0;

		virtual int getPair(const AabbUint* aabbs, int nAabbs, Pair32* pairsOut, int capacity) = 0;
};


#endif

