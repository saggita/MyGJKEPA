#pragma once

#include <Common/Geometry/Aabb.h>


class MortonKeyUtils
{
	public:
		__inline
		MortonKeyUtils(const Aabb& space);
		__inline
		u32 calcKey(const float4& p);
		__inline
		u32 calcKeyXZ(const float4& p);

	private:
		__inline
		u32 part1By2(u32 n);
		__inline
		u32 part1By1(u32 n);
		
	public:
		float4 m_min;
		float4 m_scale;
};

__inline
MortonKeyUtils::MortonKeyUtils(const Aabb& space)
{
	m_min = space.m_min;
	float4 extent = space.getExtent();
	m_scale = make_float4(1.f)/extent;
/*
	float minScale = ( m_scale.x < m_scale.y )? m_scale.x : m_scale.y;
	minScale = ( m_scale.z < minScale )? m_scale.z : minScale;
	m_scale = make_float4( minScale );
*/
}

__inline
u32 MortonKeyUtils::calcKey( const float4& p )
{
	float4 crd = (p-m_min) * m_scale * ((1 << 10) - 1);

	u32 x = (u32)crd.x;
	u32 y = (u32)crd.y;
	u32 z = (u32)crd.z;
	
	return (part1By2(z)<<2) + (part1By2(y)<<1) + part1By2(x);
}

__inline
u32 MortonKeyUtils::calcKeyXZ( const float4& p )
{
	float4 crd = (p-m_min) * m_scale * ((1 << 16) - 1);

	u32 x = (u32)crd.x;
	u32 y = (u32)crd.y;
	u32 z = (u32)crd.z;
	
	return (part1By1(z)<<1) + part1By1(x);
}

__inline
u32 MortonKeyUtils::part1By2(u32 n)
{
	n = (n^(n<<16)) &0xff0000ff;
	n = (n^(n<< 8)) &0x0300f00f;
	n = (n^(n<< 4)) &0x030c30c3;
	n = (n^(n<< 2)) &0x09249249;
	return n;
}

__inline
u32 MortonKeyUtils::part1By1(u32 n)
{
	n = (n^(n<<8)) &0x00ff00ff;
	n = (n^(n<< 4)) &0x0f0f0f0f;
	n = (n^(n<< 2)) &0x33333333;
	n = (n^(n<< 1)) &0x55555555;
	return n;
}

