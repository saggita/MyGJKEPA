#pragma once

#include <AdlPrimitives\Math\Math.h>

//	Coords are 0.5f shifted. See CubeMapDemo.cpp for usage. 
class CubeMapUtils
{
	public:
		//typedef adl::float4 float4;
		//enum Face
		//{
		//	FACE_XM,
		//	FACE_XP,
		//	FACE_YM,
		//	FACE_YP,
		//	FACE_ZM,
		//	FACE_ZP,
		//	NUM_FACES,
		//};

		__inline
		static void calcCrd(const float4& p, int& faceIdxOut, float& x, float& y);

		__inline
		static float4 calcVector(int faceIdx, float x, float y);
};


__inline
void CubeMapUtils::calcCrd(const float4& p, int& faceIdxOut, float& x, float& y)
{
	const float4 majorAxes[] = {adl::make_float4(1,0,0,0), adl::make_float4(0,1,0,0), adl::make_float4(0,0,1,0)};

	float4 majorAxis;

	{
		int idx;
		float r2[] = {p.x*p.x, p.y*p.y, p.z*p.z};

		idx = (r2[1]>r2[0])? 1:0;
		idx = (r2[2]>r2[idx])? 2:idx;
		majorAxis = majorAxes[idx];

		bool isNeg = adl::dot3F4( p, majorAxis ) < 0.f;

		faceIdxOut = (idx*2+((isNeg)? 0:1));
//==
		float4 abs = adl::make_float4( fabs(p.x), fabs(p.y), fabs(p.z), 0.f );

		float d;
		if( idx == 0 )
		{
			x = p.y;
			y = p.z;
			d = abs.x;
		}
		else if( idx == 1 )
		{
			x = p.z;
			y = p.x;
			d = abs.y;
		}
		else
		{
			x = p.x;
			y = p.y;
			d = abs.z;
		}

		float dInv = (d==0.f)? 0.f: (1.f/d);
		x = (x*dInv+1.f)*0.5f;
		y = (y*dInv+1.f)*0.5f;
	}
}

__inline
adl::float4 CubeMapUtils::calcVector(int faceIdx, float x, float y)
{
	int dir = faceIdx/2;
	float z = (faceIdx%2 == 0)? -1.f:1.f;

	x = x*2.f-1.f;
	y = y*2.f-1.f;
	
	if( dir == 0 )
	{
		return adl::make_float4(z, x, y);
	}
	else if( dir == 1 )
	{
		return adl::make_float4(y,z,x);
	}
	else
	{
		return adl::make_float4(x,y,z);
	}
}

