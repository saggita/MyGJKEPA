//#include <AdlPrimitives/Math/MathCL.h>
#include "MathCL.h"

#define WG_SIZE 64
#define HEIGHT_RES 8

typedef struct
{
	float4 m_normal[HEIGHT_RES*HEIGHT_RES*6];
	u32 m_height4[HEIGHT_RES*HEIGHT_RES*6];
	u32 m_supportHeight4[HEIGHT_RES*HEIGHT_RES*6];

	float m_scale;
	float m_padding0;
	float m_padding1;
	float m_padding2;
} ShapeData;

typedef struct
{
	u32 m_height4[HEIGHT_RES*HEIGHT_RES*6/4];

	float m_scale;
} ShapeDeviceData;

typedef struct
{
	float4 m_pos;
	float4 m_quat;
	float4 m_linVel;
	float4 m_angVel;

	u32 m_shapeIdx;
	float m_invMass;
	float m_restituitionCoeff;
	float m_frictionCoeff;
} BodyData;

typedef struct
{
	float4 m_worldPos[4];
	float4 m_worldNormal;	//	w: m_nPoints
//	float m_restituitionCoeff;
//	float m_frictionCoeff;
	u32 m_coeffs;
	u32 m_batchIdx;
//	int m_nPoints;
//	int m_padding0;

	u32 m_bodyAPtr;//x:m_bodyAPtr, y:m_bodyBPtr
	u32 m_bodyBPtr;
} Contact4;

#define GET_NPOINTS(x) (x).m_worldNormal.w


typedef struct
{
	int m_nPairs;
	float m_collisionMargin;
	int m_capacity;
	int m_paddings[1];
} ConstBuffer;

__inline
float4 transform(const float4* p, const float4* translation, const Quaternion* orientation)
{
	return qtRotate( *orientation, *p ) + (*translation);
}

__inline
float4 invTransform(const float4* p, const float4* translation, const Quaternion* orientation)
{
	return qtRotate( qtInvert( *orientation ), (*p)-(*translation) ); // use qtInvRotate
}

void CubeMapUtilsCalcCrd(const float4 p, int* faceIdxOut, float* x, float* y)
{
	{
		int idx;
		float r2[] = {p.x*p.x, p.y*p.y, p.z*p.z};

		idx = (r2[1]>r2[0])? 1:0;
		idx = (r2[2]>r2[idx])? 2:idx;

		*faceIdxOut = (idx*2);
//==
		float4 abs = make_float4( fabs(p.x), fabs(p.y), fabs(p.z), 0.f );

		float d;
		if( idx == 0 )
		{
			*x = p.y;
			*y = p.z;
			d = abs.x;
			*faceIdxOut += (p.x < 0.f)? 0: 1.f;
		}
		else if( idx == 1 )
		{
			*x = p.z;
			*y = p.x;
			d = abs.y;
			*faceIdxOut += (p.y < 0.f)? 0: 1.f;
		}
		else
		{
			*x = p.x;
			*y = p.y;
			d = abs.z;
			*faceIdxOut += (p.z < 0.f)? 0: 1.f;
		}

		float dInv = (d==0.f)? 0.f: fastDiv(1.f,d);
		*x = (*x*dInv+1.f)*0.5f;
		*y = (*y*dInv+1.f)*0.5f;
	}
}

float4 CubeMapUtilsCalcVector(int faceIdx, float x, float y)
{
	int dir = faceIdx/2;
	float z = (faceIdx%2 == 0)? -1.f:1.f;

	x = x*2.f-1.f;
	y = y*2.f-1.f;
	
	if( dir == 0 )
	{
		return make_float4(z, x, y, 0.f);
	}
	else if( dir == 1 )
	{
		return make_float4(y,z,x, 0.f);
	}
	else
	{
		return make_float4(x,y,z, 0.f);
	}
}

typedef int Face;

u32 sample(__local ShapeDeviceData* shape, int face, int x, int y)
{

	int idx = HEIGHT_RES*HEIGHT_RES*face + x + y*HEIGHT_RES;
	__local u8* height = (__local u8*)shape->m_height4;
	return height[idx];
}

u32 sampleSupportGlobal(__global ShapeData* shape, int face, int x, int y)
{

	int idx = HEIGHT_RES*HEIGHT_RES*face + x + y*HEIGHT_RES;
	__global u8* height = (__global u8*)shape->m_supportHeight4;
	return height[idx];
}

float4 sampleNormal(__local ShapeData* shape, int face, int x, int y)
{
	return shape->m_normal[HEIGHT_RES*HEIGHT_RES*face + x + y*HEIGHT_RES];
}

float4 sampleNormalGlobal(const __global ShapeData* shape, int face, int x, int y)
{
	return shape->m_normal[HEIGHT_RES*HEIGHT_RES*face + x + y*HEIGHT_RES];
}

float4 ShapeDataCalcSamplePoint( __local ShapeDeviceData* shape, int sIdx )//u8 height, int sIdx, float scale )
{
	const float oneOver255 = 1.f/255.f;

	int faceIdx = fastDiv(sIdx,(HEIGHT_RES*HEIGHT_RES));
	int r = (sIdx%(HEIGHT_RES*HEIGHT_RES));
	int i = r/HEIGHT_RES;
	int j = r%HEIGHT_RES;

	float4 v;
	float x = fastDiv((i+0.5f),(float)HEIGHT_RES);
	float y = fastDiv((j+0.5f),(float)HEIGHT_RES);
	v = CubeMapUtilsCalcVector(faceIdx, x, y);
	v = normalize3( v );

	int quantizedHeight = sample( shape, faceIdx, i, j );
	float rheight = quantizedHeight*oneOver255*shape->m_scale;
	return rheight*v;
}

float ShapeDataQueryDistance(__local ShapeDeviceData* shape, float4 p )
{
	if( dot3F4( p, p ) >= shape->m_scale*shape->m_scale ) return FLT_MAX;

	const float oneOver255 = 1.f/255.f;

	int faceIdx;
	float x, y;
	CubeMapUtilsCalcCrd( p, &faceIdx, &x, &y );
	x = (x*HEIGHT_RES) - 0.5f;
	y = (y*HEIGHT_RES) - 0.5f;

	float height;
	{
		int xi = (int)(x);
		int yi = (int)(y);
		float dx = x-xi;
		float dy = y-yi;

		{
			int xip = min2((int)(HEIGHT_RES-1), xi+1);
			int yip = min2((int)(HEIGHT_RES-1), yi+1);

			u32 xy = sample( shape, faceIdx, xi, yi );
			u32 xpy = sample( shape, faceIdx, xip, yi );
			u32 xpyp = sample( shape, faceIdx, xip, yip );
			u32 xyp = sample( shape, faceIdx, xi, yip );

			height = (xy*(1.f-dx)+xpy*dx)*(1.f-dy) + (xyp*(1.f-dx)+xpyp*dx)*dy;
			height = height*oneOver255*shape->m_scale;

			p.w = 0.f;

			height = fastLength4( p ) - height;
		}
	}

	return height;
}

float ShapeDataQuerySupportHeight(__global ShapeData* shape, float4 p )
{
	int faceIdx;
	float x, y;
	CubeMapUtilsCalcCrd( p, &faceIdx, &x, &y );
	x = (x*HEIGHT_RES) - 0.5f;
	y = (y*HEIGHT_RES) - 0.5f;

	float height;
	{
		int xi = (int)(x);
		int yi = (int)(y);

		{
			int xip = min2((int)(HEIGHT_RES-1), xi+1);
			int yip = min2((int)(HEIGHT_RES-1), yi+1);

			u32 xy = sampleSupportGlobal( shape, faceIdx, xi, yi );
			u32 xpy = sampleSupportGlobal( shape, faceIdx, xip, yi );
			u32 xpyp = sampleSupportGlobal( shape, faceIdx, xip, yip );
			u32 xyp = sampleSupportGlobal( shape, faceIdx, xi, yip );

			height = max2( xy, max2( xpy, max2( xpyp, xyp ) ) );
			height = height/255.f*shape->m_scale;
		}
	}

	return height;

}

float4 ShapeDataQueryNormal(const __global ShapeData* shape,  float4 p )
{
	int faceIdx;
	float x, y;
	CubeMapUtilsCalcCrd( p, &faceIdx, &x, &y );
	x = (x*HEIGHT_RES) - 0.5f;
	y = (y*HEIGHT_RES) - 0.5f;

	float4 normalOut;
	{
		int xi = (int)(x);
		int yi = (int)(y);

		normalOut = sampleNormalGlobal( shape, faceIdx, xi, yi );
	}
	return normalOut;
}



//	kernels


__kernel
__attribute__((reqd_work_group_size(WG_SIZE,1,1)))
void SupportCullingKernel( __global int2* restrict gPairsIn, __global ShapeData* gShapes, 
		__global BodyData* gBodies, 
		__global int2* gPairsOut, 
		counter32_t gNPairs,
		ConstBuffer cb )
{
	int gIdx = GET_GLOBAL_IDX;
	if( gIdx >= cb.m_nPairs ) return;

	const float collisionMargin = cb.m_collisionMargin;
	const int capacity = cb.m_capacity;

	int2 pair = gPairsIn[gIdx];
	BodyData bodyA = gBodies[pair.x];
	BodyData bodyB = gBodies[pair.y];
	int shapeAIdx = bodyA.m_shapeIdx;
	int shapeBIdx = bodyB.m_shapeIdx;


	bool collide;
	{
		float4 abInA, baInB;
		float4 ab = bodyB.m_pos - bodyA.m_pos;
		{
			abInA = qtInvRotate( bodyA.m_quat, ab );
			baInB = qtInvRotate( bodyB.m_quat, -ab );
		}
		float hA = ShapeDataQuerySupportHeight( gShapes+shapeAIdx, abInA );
		float hB = ShapeDataQuerySupportHeight( gShapes+shapeBIdx, baInB );

		float h2 = dot3F4( ab, ab );

		collide = ( hA + hB + collisionMargin > sqrtf(h2) );
	}

	if( collide )
	{
		int dstIdx;
		AppendInc( gNPairs, dstIdx );
		if( dstIdx < capacity )
			gPairsOut[dstIdx] = pair;
	}
}


#define PARALLEL_DO(execution, n) for(int ie=0; ie<n; ie++){execution;}
#define PARALLEL_REDUCE_MAX32(h) \
	{int lIdx = GET_LOCAL_IDX;\
	if( lIdx < 32 )\
	{\
		h[lIdx] = (h[lIdx].y > h[lIdx+1].y)? h[lIdx]: h[lIdx+1];\
		mem_fence( CLK_LOCAL_MEM_FENCE );\
		h[lIdx] = (h[lIdx].y > h[lIdx+2].y)? h[lIdx]: h[lIdx+2];\
		mem_fence( CLK_LOCAL_MEM_FENCE );\
		h[lIdx] = (h[lIdx].y > h[lIdx+4].y)? h[lIdx]: h[lIdx+4];\
		mem_fence( CLK_LOCAL_MEM_FENCE );\
		h[lIdx] = (h[lIdx].y > h[lIdx+8].y)? h[lIdx]: h[lIdx+8];\
		mem_fence( CLK_LOCAL_MEM_FENCE );\
		h[lIdx] = (h[lIdx].y > h[lIdx+16].y)? h[lIdx]: h[lIdx+16];\
	}}

#define PARALLEL_REDUCE32(h) \
	{int lIdx = GET_LOCAL_IDX;\
		if( lIdx < 32 )\
		{\
			h[lIdx] += h[lIdx+1];\
			mem_fence( CLK_LOCAL_MEM_FENCE );\
			h[lIdx] += h[lIdx+2];\
			mem_fence( CLK_LOCAL_MEM_FENCE );\
			h[lIdx] += h[lIdx+4];\
			mem_fence( CLK_LOCAL_MEM_FENCE );\
			h[lIdx] += h[lIdx+8];\
			mem_fence( CLK_LOCAL_MEM_FENCE );\
			h[lIdx] += h[lIdx+16];\
		}}


float4 extractManifold(__local float4* p, __local float4* h, __local int* nPointsPtr, float4 nearNormal)
{
	int nPoints = *nPointsPtr;
	float4 center = make_float4(0,0,0,0);
	{	//	calculate center
		nPoints = min2( nPoints, 32 );
		{
			int lIdx = GET_LOCAL_IDX;
			h[lIdx] = p[lIdx];
			h[lIdx] = (lIdx<nPoints)? h[lIdx] : make_float4(0,0,0,0);
		}
		GROUP_LDS_BARRIER;

		PARALLEL_REDUCE32( h );//working on h[64]

		GROUP_LDS_BARRIER;

//		if( GET_LOCAL_IDX == 0 )
		{
			center = fastDiv4( h[0], make_float4(nPoints, nPoints, nPoints, 0.f) );
		}
		GROUP_LDS_BARRIER;

		if( nPoints < 4 ) return center;
	}
	//	is center set on all the WIs?
	float4 aVector = p[0] - center;
	float4 u = normalize3( cross3( nearNormal, aVector ) );
	float4 v = normalize3( cross3( nearNormal, u ) );

	int idx[4];

	__local int4* a = (__local int4*)h;
	{	//	select 4
		{	//	set dot of 4 directions for xyzw
			int ie = GET_LOCAL_IDX;
			{
				float f;
				float4 r = p[ie]-center;
				f = dot3F4( u, r );
				a[ie].x = ((*(u32*)&f) & 0xffffff00) | (0xff & ie);

				f = dot3F4( -u, r );
				a[ie].y = ((*(u32*)&f) & 0xffffff00) | (0xff & ie);

				f = dot3F4( v, r );
				a[ie].z = ((*(u32*)&f) & 0xffffff00) | (0xff & ie);

				f = dot3F4( -v, r );
				a[ie].w = ((*(u32*)&f) & 0xffffff00) | (0xff & ie);

				if( ie >= nPoints ) a[ie] = make_int4(-0xfffffff, -0xfffffff, -0xfffffff, -0xfffffff);
			}
		}

		GROUP_LDS_BARRIER;

		{	//	vector reduce, h[64]
			int lIdx = GET_LOCAL_IDX;
			if( lIdx < 32 )
			{
				h[lIdx] = max2( h[lIdx], h[lIdx+1] );
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] = max2( h[lIdx], h[lIdx+2] );
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] = max2( h[lIdx], h[lIdx+4] );
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] = max2( h[lIdx], h[lIdx+8] );
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] = max2( h[lIdx], h[lIdx+16] );
			}
		}

		GROUP_LDS_BARRIER;
	}
	{
		{	//	set to idx
			idx[0] = (int)a[0].x & 0xff;
			idx[1] = (int)a[0].y & 0xff;
			idx[2] = (int)a[0].z & 0xff;
			idx[3] = (int)a[0].w & 0xff;
		}

		GROUP_LDS_BARRIER;
		float4 selection;
		if( GET_LOCAL_IDX < 4 ) selection = p[idx[GET_LOCAL_IDX]];

		GROUP_LDS_BARRIER;
		if( GET_LOCAL_IDX < 4 ) p[GET_LOCAL_IDX] = selection;
	}


	return center;
}

void extractManifold1(__local float4* p, __local float4* h, __local int* nPointsPtr, float4 center)
{
	__local int* a = (__local int*)h;
	{
		GROUP_LDS_BARRIER;
		float4 selection;
		if( GET_LOCAL_IDX < 4 )
		{
			int idx = (int)a[GET_LOCAL_IDX] & 0xff;
			selection = p[idx];
		}

		GROUP_LDS_BARRIER;
		if( GET_LOCAL_IDX < 4 ) p[GET_LOCAL_IDX] = selection;
	}

}

void extractManifold2(	__local float4* p0, __local int* nPointsPtr0, float4 nearNormal0,
						__local float4* p1, __local int* nPointsPtr1, float4 nearNormal1,
						__local float4* h, float4 centerOut[2])
{

	int nPoints[2];
	nPoints[0] = *nPointsPtr0;
	nPoints[1] = *nPointsPtr1;
	float4 center[2];
	center[0] = make_float4(0,0,0,0);
	center[1] = make_float4(0,0,0,0);
	{	//	calculate center
		nPoints[0] = min2( nPoints[0], 32 );
		nPoints[1] = min2( nPoints[1], 32 );
		{
			int lIdx = GET_LOCAL_IDX;
			h[lIdx] = (lIdx<nPoints[0])? p0[lIdx] : make_float4(0,0,0,0);
			h[lIdx+64] = (lIdx<nPoints[1])? p1[lIdx] : make_float4(0,0,0,0);
		}
		GROUP_LDS_BARRIER;

		{
			int bIdx = GET_LOCAL_IDX/32;
			int eIdx = GET_LOCAL_IDX%32;
			int lIdx = eIdx + bIdx*64;
			{
				h[lIdx] += h[lIdx+1];
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] += h[lIdx+2];
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] += h[lIdx+4];
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] += h[lIdx+8];
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] += h[lIdx+16];
			}
		}

		GROUP_LDS_BARRIER;

		for(int bIdx=0; bIdx<2; bIdx++)
		{
			center[bIdx] = fastDiv4( h[bIdx*64], make_float4(nPoints[bIdx], nPoints[bIdx], nPoints[bIdx], 0.f) );
		}
		GROUP_LDS_BARRIER;
	}

	centerOut[0] = center[0];
	centerOut[1] = center[1];

	float4 u[2];
	float4 v[2];

	{
		float4 aVector = p0[0] - center[0];
		u[0] = normalize3( cross3( nearNormal0, aVector ) );
		v[0] = normalize3( cross3( nearNormal0, u[0] ) );
	}
	{
		float4 aVector = p1[0] - center[1];
		u[1] = normalize3( cross3( nearNormal1, aVector ) );
		v[1] = normalize3( cross3( nearNormal1, u[1] ) );
	}

	{
		__local int4* a = (__local int4*)h;
		{	//	select 4
			{	//	set dot of 4 directions for xyzw
				int ie = GET_LOCAL_IDX%32;
				int setIdx = GET_LOCAL_IDX/32;
				{
					float f;
					float4 r = p0[ie + setIdx*32]-center[setIdx];
					f = dot3F4( u[setIdx], r );
					a[ie + setIdx*64].x = ((*(u32*)&f) & 0xffffff00) | (0xff & ie);

					f = dot3F4( -u[setIdx], r );
					a[ie + setIdx*64].y = ((*(u32*)&f) & 0xffffff00) | (0xff & ie);

					f = dot3F4( v[setIdx], r );
					a[ie + setIdx*64].z = ((*(u32*)&f) & 0xffffff00) | (0xff & ie);

					f = dot3F4( -v[setIdx], r );
					a[ie + setIdx*64].w = ((*(u32*)&f) & 0xffffff00) | (0xff & ie);

					if( ie >= nPoints[setIdx] ) a[ie + setIdx*64] = make_int4(-0xfffffff, -0xfffffff, -0xfffffff, -0xfffffff);

					a[ie + 32] = make_int4(-0xfffffff, -0xfffffff, -0xfffffff, -0xfffffff);
				}
			}
		}
		GROUP_LDS_BARRIER;

		{	//	vector reduce, h[64]
			int bIdx = GET_LOCAL_IDX/32;
			int eIdx = GET_LOCAL_IDX%32;
			int lIdx = eIdx + bIdx*64;
			{
				h[lIdx] = max2( h[lIdx], h[lIdx+1] );
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] = max2( h[lIdx], h[lIdx+2] );
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] = max2( h[lIdx], h[lIdx+4] );
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] = max2( h[lIdx], h[lIdx+8] );
				mem_fence( CLK_LOCAL_MEM_FENCE );
				h[lIdx] = max2( h[lIdx], h[lIdx+16] );
			}
		}

		GROUP_LDS_BARRIER;
	}
	__local int* a = (__local int*)h;
	{
		GROUP_LDS_BARRIER;
		
		float4 selection;

		int bIdx = GET_LOCAL_IDX/32;
		int eIdx = GET_LOCAL_IDX%32;

		if( eIdx < 4 )
		{
			int idx = (int)a[eIdx+64*4*bIdx] & 0xff;
			selection = p0[idx+32*bIdx];
		}

		GROUP_LDS_BARRIER;
		if( eIdx < 4 ) p0[eIdx+32*bIdx] = selection;
	}
}

/*
1. Query Normal
2. Fill Normal
3. A->B, B->A
*/

void testVtx(__local BodyData* bodyAPtr, __local BodyData* bodyBPtr,
			__local ShapeDeviceData* shapeAPtr, __local ShapeDeviceData* shapeBPtr,
			__local int* lNContacts, __local float4* lCPoints)
{
	int pIdx = GET_LOCAL_IDX;
	float4 bodyAPos = bodyAPtr->m_pos;
	float4 bodyBPos = bodyBPtr->m_pos;
	Quaternion bodyAQuat = bodyAPtr->m_quat;
	Quaternion bodyBQuat = bodyBPtr->m_quat;
	while( pIdx < HEIGHT_RES*HEIGHT_RES*6 )
	{
		float4 pInB = ShapeDataCalcSamplePoint( shapeBPtr, pIdx );

		float4 pInW = transform( &pInB, &bodyBPos, &bodyBQuat );
//		Aabb bodyAAabb = bodyAPtr->m_aabb;
//		if( AabbOverlapsPoint( &bodyAAabb, pInW ) )
		{
			float4 pInA = invTransform( &pInW, &bodyAPos, &bodyAQuat );

			float dist = ShapeDataQueryDistance( shapeAPtr, pInA );
			if( dist < 0.010f )
			{
				int dstIdx = atom_add( lNContacts, 1 );
				if( dstIdx < 32 )
				{
					lCPoints[ dstIdx ] = make_float4( pInA.x, pInA.y, pInA.z, dist );
				}
			}
		}

		pIdx += GET_GROUP_SIZE;
	}
}

void testVtx2(__local const BodyData* bodyA, __local const BodyData* bodyB,
			__local const ShapeDeviceData* shapeA, __local const ShapeDeviceData* shapeB,
			__local int* lNContactsA, __local float4* lCPointsA,
			__local int* lNContactsB, __local float4* lCPointsB, float collisionMargin )
{
	int pIdx = GET_LOCAL_IDX;

	while( pIdx < HEIGHT_RES*HEIGHT_RES*6*2 )
	{
		__local BodyData* bodyAPtr			=( pIdx < HEIGHT_RES*HEIGHT_RES*6 )?bodyA:bodyB;
		__local BodyData* bodyBPtr			=( pIdx < HEIGHT_RES*HEIGHT_RES*6 )?bodyB:bodyA;
		__local ShapeDeviceData* shapeAPtr	=( pIdx < HEIGHT_RES*HEIGHT_RES*6 )?shapeA:shapeB;
		__local ShapeDeviceData* shapeBPtr	=( pIdx < HEIGHT_RES*HEIGHT_RES*6 )?shapeB:shapeA;
		__local int* lNContacts				=( pIdx < HEIGHT_RES*HEIGHT_RES*6 )?lNContactsA:lNContactsB;
		__local float4* lCPoints			=( pIdx < HEIGHT_RES*HEIGHT_RES*6 )?lCPointsA:lCPointsB;

		float4 bodyAPos = bodyAPtr->m_pos;
		float4 bodyBPos = bodyBPtr->m_pos;
		Quaternion bodyAQuat = bodyAPtr->m_quat;
		Quaternion bodyBQuat = bodyBPtr->m_quat;

		float4 pInB = ShapeDataCalcSamplePoint( shapeBPtr, pIdx%(HEIGHT_RES*HEIGHT_RES*6) );

		float4 pInW = transform( &pInB, &bodyBPos, &bodyBQuat );
//		Aabb bodyAAabb = bodyAPtr->m_aabb;
//		if( AabbOverlapsPoint( &bodyAAabb, pInW ) )
		{
			float4 pInA = invTransform( &pInW, &bodyAPos, &bodyAQuat );

			float dist = ShapeDataQueryDistance( shapeAPtr, pInA );
			if( dist < collisionMargin )
			{
				int dstIdx = atom_add( lNContacts, 1 );
				if( dstIdx < 32 )
				{
					lCPoints[ dstIdx ] = make_float4( pInA.x, pInA.y, pInA.z, dist );
				}
			}
		}

		pIdx += GET_GROUP_SIZE;
	}
}

void testVtxWithPlane(__local BodyData* bodyA, __local BodyData* bodyB,
			float4 nA, __local ShapeDeviceData* shapeB,
			__local int* lNContactsA, __local float4* lCPointsA, float collisionMargin)
{
	int pIdx = GET_LOCAL_IDX;

	while( pIdx < HEIGHT_RES*HEIGHT_RES*6 )
	{
		__local BodyData* bodyAPtr			=bodyA;
		__local BodyData* bodyBPtr			=bodyB;
		__local ShapeDeviceData* shapeBPtr	=shapeB;
		__local int* lNContacts				=lNContactsA;
		__local float4* lCPoints				=lCPointsA;

		float4 bodyAPos = bodyAPtr->m_pos;
		float4 bodyBPos = bodyBPtr->m_pos;
		Quaternion bodyAQuat = bodyAPtr->m_quat;
		Quaternion bodyBQuat = bodyBPtr->m_quat;

		float4 pInB = ShapeDataCalcSamplePoint( shapeBPtr, pIdx%(HEIGHT_RES*HEIGHT_RES*6) );

		float4 pInW = transform( &pInB, &bodyBPos, &bodyBQuat );
		{
			float4 pInA = invTransform( &pInW, &bodyAPos, &bodyAQuat );

			float dist = dot3w1( pInA, nA );//ShapeDataQueryDistance( shapeAPtr, pInA );
			if( dist < collisionMargin )
			{
				int dstIdx = atom_add( lNContacts, 1 );
				if( dstIdx < 32 )
				{
					lCPoints[ dstIdx ] = make_float4( pInA.x, pInA.y, pInA.z, dist );
				}
			}
		}

		pIdx += GET_GROUP_SIZE;
	}
}

#define GET_SHAPE_IDX(x) (int)((x).m_shapeIdx)

void output(__local BodyData* bodyAPtr, __local BodyData* bodyBPtr,
			__local int2* iPair,
			__local int* lNContacts, __local float4* lCPoints,
			float4 center, 
			__global ShapeData* shapeData, __global Contact4* contactsOut, float collisionMargin)
{
	if( *lNContacts != 0 )
	{
		int nContacts = min2( *lNContacts, 4 );

		__global Contact4* c = contactsOut;

		if( GET_LOCAL_IDX < nContacts )
		{
			int i = GET_LOCAL_IDX;
			float4 p = lCPoints[i];
			float4 bodyAPos = bodyAPtr->m_pos;
			Quaternion bodyAQuat = bodyAPtr->m_quat;

			c->m_worldPos[i] = transform( &p, &bodyAPos, &bodyAQuat );
			c->m_worldPos[i].w = lCPoints[i].w - collisionMargin;
		}

		if( GET_LOCAL_IDX == 0 )
		{
			float4 contactNormal;
			contactNormal = ShapeDataQueryNormal( &shapeData[GET_SHAPE_IDX(*bodyAPtr)], center );
			contactNormal = normalize3( qtRotate( bodyAPtr->m_quat, contactNormal ) );

			c->m_worldNormal = contactNormal;
//			c->m_restituitionCoeff = 0.f;
//			c->m_frictionCoeff = 0.7f;
			c->m_coeffs = (u32)(0.f*0xffff) | ((u32)(0.7f*0xffff)<<16);
			GET_NPOINTS(*c) = nContacts;
			c->m_bodyAPtr = iPair[0].x;
			c->m_bodyBPtr = iPair[0].y;
		}
	}
	else
	{
		if( GET_LOCAL_IDX == 0 )
			GET_NPOINTS(contactsOut[0]) = 0;
	}
}

//	todo. make it better
void output2(__local BodyData* bodyAPtr, __local BodyData* bodyBPtr,
			int pair0, int pair1,
			__local int* lNContacts, __local float4* lCPoints,
			float4 center, 
			const __global ShapeData* shapeData, __global Contact4* contactsOut, counter32_t nContactsOut, int capacity,
			float collisionMargin )
{
	int lIdx = GET_LOCAL_IDX%32;
	int nContacts = min2( *lNContacts, 4 );
	
	GROUP_LDS_BARRIER;

	if( lIdx == 0 && nContacts)
	{
		int dstIdx;
		AppendInc( nContactsOut, dstIdx );
		*lNContacts = dstIdx;

		if( dstIdx >= capacity )
			*lNContacts = -1;
	}

	GROUP_LDS_BARRIER;

	bool canWrite = (*lNContacts!=-1);

	if( nContacts && canWrite )
	{
		__global Contact4* c = contactsOut + (*lNContacts);

		if( lIdx < nContacts )
		{
			int i = lIdx;
			float4 p = lCPoints[i];
			float4 bodyAPos = bodyAPtr->m_pos;
			Quaternion bodyAQuat = bodyAPtr->m_quat;

			p = transform( &p, &bodyAPos, &bodyAQuat );
			p.w = lCPoints[i].w - collisionMargin;
			c->m_worldPos[i] = p;
		}

		if( lIdx == 0 )
		{
			if( nContacts )
			{
				float4 contactNormal;
				contactNormal = ShapeDataQueryNormal( &shapeData[GET_SHAPE_IDX(*bodyAPtr)], center );
				contactNormal = normalize3( qtRotate( bodyAPtr->m_quat, contactNormal ) );

				c->m_worldNormal = contactNormal;
//				c->m_restituitionCoeff = 0.f;
//				c->m_frictionCoeff = 0.7f;
				c->m_coeffs = (u32)(0.f*0xffff) | ((u32)(0.7f*0xffff)<<16);
				c->m_bodyAPtr = pair0;
				c->m_bodyBPtr = pair1;
			}
			GET_NPOINTS(*c) = nContacts;
		}
	}
}

__inline
void output2LDS(__local BodyData* bodyAPtr, __local BodyData* bodyBPtr,
			int pair0, int pair1,
			int lNContacts, __local float4* lCPoints,
			float4 center, 
			const __global ShapeData* shapeData, __local Contact4* contactsOut,
			float collisionMargin )
{
	int lIdx = GET_LOCAL_IDX%32;
//	int lIdx = GET_LOCAL_IDX;
//	int groupIdx = 0;

	int nContacts = min2( lNContacts, 4 );
	
	GROUP_LDS_BARRIER;

	if( nContacts != 0  )
	{
		if( lIdx < nContacts )
		{
			int i = lIdx;
			float4 p = lCPoints[i];
			float4 bodyAPos = bodyAPtr->m_pos;
			Quaternion bodyAQuat = bodyAPtr->m_quat;

			p = transform( &p, &bodyAPos, &bodyAQuat );
			p.w = lCPoints[i].w - collisionMargin;
			contactsOut->m_worldPos[i] = p;
		}
	}

	if( lIdx == 0 )
	{
		if( nContacts != 0 )
		{
			float4 contactNormal;
			contactNormal = ShapeDataQueryNormal( &shapeData[GET_SHAPE_IDX(*bodyAPtr)], center );
			contactNormal = normalize3( qtRotate( bodyAPtr->m_quat, contactNormal ) );

			contactsOut->m_worldNormal = contactNormal;
//			contactsOut->m_worldNormal = make_float4(1.5f,1.4f,1.3f,0.f);
//			contactsOut->m_restituitionCoeff = 0.f;
//			contactsOut->m_frictionCoeff = 0.7f;
			contactsOut->m_coeffs = (u32)(0.f*0xffff) | ((u32)(0.7f*0xffff)<<16);
			contactsOut->m_bodyAPtr = pair0;
			contactsOut->m_bodyBPtr = pair1;
		}
		GET_NPOINTS(*contactsOut) = nContacts;//nContacts;
	}

//	contactsOut[groupIdx].m_worldNormal = make_float4(1.5f,1.4f,1.3f,0.f);
}

void output2_1(__local BodyData* bodyAPtr, __local BodyData* bodyBPtr,
			int pair0, int pair1,
			__local int* lNContacts, __local float4* lCPoints,
			float4 center, float4 nA, 
			const __global ShapeData* shapeData, __global Contact4* contactsOut, counter32_t nContactsOut, int capacity, float collisionMargin )
{
	int lIdx = GET_LOCAL_IDX;
	int nContacts = min2( *lNContacts, 4 );
	
	GROUP_LDS_BARRIER;

	if( lIdx == 0 && nContacts)
	{
		int dstIdx;
		AppendInc( nContactsOut, dstIdx );
		*lNContacts = dstIdx;

		if( dstIdx >= capacity )
			*lNContacts = -1;
	}

	GROUP_LDS_BARRIER;

	bool canWrite = (*lNContacts!=-1);

	if( nContacts && canWrite )
	{
		__global Contact4* c = contactsOut + (*lNContacts);

		if( lIdx < nContacts )
		{
			int i = lIdx;
			float4 p = lCPoints[i];
			float4 bodyAPos = bodyAPtr->m_pos;
			Quaternion bodyAQuat = bodyAPtr->m_quat;

			p = transform( &p, &bodyAPos, &bodyAQuat );
			p.w = lCPoints[i].w - collisionMargin;
			c->m_worldPos[i] = p;
		}

		if( lIdx == 0 )
		{
			if( nContacts )
			{
				float4 contactNormal;
				contactNormal = nA;//ShapeDataQueryNormal( &shapeData[GET_SHAPE_IDX(*bodyAPtr)], center );
				contactNormal = normalize3( qtRotate( bodyAPtr->m_quat, contactNormal ) );

				c->m_worldNormal = contactNormal;
//				c->m_restituitionCoeff = 0.f;
//				c->m_frictionCoeff = 0.7f;
				c->m_coeffs = (u32)(0.f*0xffff) | ((u32)(0.7f*0xffff)<<16);
				c->m_bodyAPtr = pair0;
				c->m_bodyBPtr = pair1;
			}
			GET_NPOINTS(*c) = nContacts;
		}
	}
}

__kernel
void manifold(__global float4* vIn, __global float4* vOut)
{
	__local float4 lCPoints[32];
	__local float4 lManifoldBuffer[64];
	__local int lNContacts;
	__local float4 ab;

	if( GET_LOCAL_IDX<32 )
	{
		lCPoints[GET_LOCAL_IDX] = vIn[GET_GLOBAL_IDX];
	}

	if( GET_LOCAL_IDX == 0 ) 
	{
		lNContacts = 32;
		ab = vIn[GET_GLOBAL_IDX];
	}

	GROUP_LDS_BARRIER;

	float4 center = extractManifold( lCPoints, lManifoldBuffer, &lNContacts, ab );

	if( GET_LOCAL_IDX < lNContacts )
	{
		vOut[4*GET_GROUP_IDX+GET_LOCAL_IDX] = lCPoints[GET_LOCAL_IDX];
	}

}

//#define COMBINE_REDUCTION 

__kernel
__attribute__((reqd_work_group_size(64, 1, 1)))
void NarrowphaseKernel( const __global int2* restrict pairs, const __global ShapeData* shapeData, const __global BodyData* restrict bodyDatas, 
					   __global Contact4* restrict contactsOut,
					   counter32_t nContactsOut, ConstBuffer cb ) 
{
	//	2.5K LDS
	__local Contact4 ldsContacts[2];
	__local BodyData bodyA;
	__local BodyData bodyB;
	__local ShapeDeviceData shapeA;
	__local ShapeDeviceData shapeB;
	__local float4 lCPointsA[32*2];
	__local int lNContactsA;
	__local float4* lCPointsB = lCPointsA+32;
	__local int lNContactsB;
#ifdef COMBINE_REDUCTION
	__local float4 lManifoldBuffer[64*2];
#else
	__local float4 lManifoldBuffer[64];
#endif
	__local int2 iPairAB;

	const int capacity = cb.m_capacity;
	const float collisionMargin = cb.m_collisionMargin;


	int pairIdx = GET_GROUP_IDX;
//	for(int pairIdx = GET_GROUP_IDX; pairIdx<nPairs; pairIdx+=GET_NUM_GROUPS)
	{
		if( GET_LOCAL_IDX == 0 )	//	load Bodies
		{
			int2 pair = pairs[pairIdx];
			iPairAB = make_int2(pair.x, pair.y);
			bodyA = bodyDatas[ pair.x ];
			bodyB = bodyDatas[ pair.y ];
			shapeA.m_scale = shapeData[ GET_SHAPE_IDX(bodyA) ].m_scale;
			shapeB.m_scale = shapeData[ GET_SHAPE_IDX(bodyB) ].m_scale;
			lNContactsA = 0;
			lNContactsB = 0;
		}
		
		GROUP_LDS_BARRIER;

		//	todo. can check if the shape is the same to previous one. If same, dont read
		{	//	load shape data
			int idx = GET_LOCAL_IDX%32;
			int bIdx = GET_LOCAL_IDX/32;
			__local ShapeDeviceData* myShape = (bIdx==0)?&shapeA: &shapeB;
			int myShapeIdx = (bIdx==0)?GET_SHAPE_IDX(bodyA): GET_SHAPE_IDX(bodyB);

			while( idx < HEIGHT_RES*HEIGHT_RES*6/4 )
			{
				myShape->m_height4[idx] = shapeData[ myShapeIdx ].m_height4[idx];

				idx+=32;
			}
		}

		GROUP_LDS_BARRIER;

		testVtx2( &bodyA, &bodyB, &shapeA, &shapeB, &lNContactsA, lCPointsA, &lNContactsB, lCPointsB, collisionMargin );

		GROUP_LDS_BARRIER;

		float4 ab = bodyB.m_pos - bodyA.m_pos;
		float4 center[2];

		if( lNContactsA != 0 || lNContactsB != 0 )
		{
			float4 abInA;
			abInA = qtInvRotate( bodyA.m_quat, ab );

			float4 abInB;
			abInB = qtInvRotate( bodyB.m_quat, ab );

#ifdef COMBINE_REDUCTION
			extractManifold2( lCPointsA, &lNContactsA, abInA,
				lCPointsB, &lNContactsB, abInB,
				lManifoldBuffer, center );
#else
			if( lNContactsA != 0 )
				center[0] = extractManifold( lCPointsA, lManifoldBuffer, &lNContactsA, abInA );
			if(  lNContactsB != 0 )
				center[1] = extractManifold( lCPointsB, lManifoldBuffer, &lNContactsB, abInB );
#endif
		}

		int firstSet = GET_LOCAL_IDX/32;

/*
		if( GET_LOCAL_IDX == 0 )	//	for debug
		{
			ldsContacts[0].m_worldNormal = make_float4(-1,-1,-1,0);
			ldsContacts[0].m_bodyAPtr = 0;
			ldsContacts[0].m_bodyBPtr = 0;
			ldsContacts[0].m_batchIdx = 111;
			ldsContacts[1].m_worldNormal = make_float4(-1,-1,-1,0);
			ldsContacts[1].m_bodyAPtr = 0;
			ldsContacts[1].m_bodyBPtr = 0;
			ldsContacts[1].m_batchIdx = 111;
		}
*/
		bool doReduction = true;
		if( doReduction )
		{
			GROUP_LDS_BARRIER;

			output2LDS( (firstSet)?&bodyA: &bodyB, (firstSet)?&bodyB : &bodyA, 
				(firstSet)?iPairAB.x : iPairAB.y, (firstSet)?iPairAB.y : iPairAB.x, 
				(firstSet)?lNContactsA : lNContactsB, (firstSet)?lCPointsA:lCPointsB, 
				(firstSet)?center[0] : center[1], shapeData, (firstSet)?&ldsContacts[0]: &ldsContacts[1], collisionMargin );

			GROUP_LDS_BARRIER;
		
			if( GET_LOCAL_IDX == 0 )
			{
				if( lNContactsA && lNContactsB )
				{
					float nDotn = dot3F4( ldsContacts[0].m_worldNormal, ldsContacts[1].m_worldNormal );
					if( nDotn < -(1.f-0.01f) )
					{
						if( ldsContacts[0].m_bodyAPtr > ldsContacts[1].m_bodyAPtr )
							lNContactsA = 0;
						else
							lNContactsB = 0;
					}
				}
			}
		
			if( GET_LOCAL_IDX == 0 )
			{
				int n = lNContactsA;
				if( n != 0 )
				{
					int dstIdx;
					AppendInc( nContactsOut, dstIdx );
					if( dstIdx < capacity )
					{	int idx = 0;
						contactsOut[ dstIdx ] = ldsContacts[idx];
					}
				}

				n = lNContactsB;
				if( n != 0 )
				{
					int dstIdx;
					AppendInc( nContactsOut, dstIdx );
					if( dstIdx < capacity )
					{	int idx = 1;
						contactsOut[ dstIdx ] = ldsContacts[idx];
					}
				}
			}

			GROUP_LDS_BARRIER;
		}
		else
		{
			output2( (firstSet)?&bodyA: &bodyB, (firstSet)?&bodyB : &bodyA, 
				(firstSet)?iPairAB.x : iPairAB.y, (firstSet)?iPairAB.y : iPairAB.x, 
				(firstSet)?&lNContactsA : &lNContactsB, (firstSet)?lCPointsA:lCPointsB, 
				(firstSet)?center[0] : center[1], shapeData, contactsOut, nContactsOut, capacity, collisionMargin );
		}
	}
}


__kernel
__attribute__((reqd_work_group_size(64, 1, 1)))
void NarrowphaseWithPlaneKernel( const __global int2* restrict pairs, const __global ShapeData* shapeData, const __global BodyData* restrict bodyDatas, 
					   __global Contact4* restrict contactsOut,
					   counter32_t nContactsOut, ConstBuffer cb ) 
{
	//	2.5K LDS
	__local BodyData bodyA;
	__local BodyData bodyB;
	__local ShapeDeviceData shapeA;
	__local ShapeDeviceData shapeB;
	__local float4 lCPointsA[32*2];
	__local int lNContactsA;
//	__local float4* lCPointsB = lCPointsA+32;
//	__local int lNContactsB;
	__local float4 lManifoldBuffer[64];
	__local int2 iPairAB;

	const int capacity = cb.m_capacity;
	const float collisionMargin = cb.m_collisionMargin;

	int pairIdx = GET_GROUP_IDX;
	{
		if( GET_LOCAL_IDX == 0 )	//	load Bodies
		{
			int2 pair = pairs[pairIdx];
			iPairAB = make_int2(pair.x, pair.y);
			bodyA = bodyDatas[ pair.x ];
			bodyB = bodyDatas[ pair.y ];
			shapeA.m_scale = shapeData[ GET_SHAPE_IDX(bodyA) ].m_scale;
			shapeB.m_scale = shapeData[ GET_SHAPE_IDX(bodyB) ].m_scale;
			lNContactsA = 0;
//			lNContactsB = 0;
		}
		
		GROUP_LDS_BARRIER;

		//	todo. can check if the shape is the same to previous one. If same, dont read
		{	//	load shape data
			int idx = GET_LOCAL_IDX%32;
			int bIdx = GET_LOCAL_IDX/32;
			__local ShapeDeviceData* myShape = (bIdx==0)?&shapeA: &shapeB;
			int myShapeIdx = (bIdx==0)?GET_SHAPE_IDX(bodyA): GET_SHAPE_IDX(bodyB);

			while( idx < HEIGHT_RES*HEIGHT_RES*6/4 )
			{
				myShape->m_height4[idx] = shapeData[ myShapeIdx ].m_height4[idx];

				idx+=32;
			}
		}

		GROUP_LDS_BARRIER;

		float4 nA = make_float4(0,1,0,0);

//		testVtx2( &bodyA, &bodyB, &shapeA, &shapeB, &lNContactsA, lCPointsA, &lNContactsB, lCPointsB );
		testVtxWithPlane( &bodyA, &bodyB, nA, &shapeB, &lNContactsA, lCPointsA, collisionMargin );

		GROUP_LDS_BARRIER;

//		float4 ab = bodyB.m_pos - bodyA.m_pos;
		float4 center[2];

		if( lNContactsA != 0 )
		{
			float4 abInA;
			abInA = nA;//qtInvRotate( bodyA.m_quat, ab );

			if( lNContactsA != 0 )
				center[0] = extractManifold( lCPointsA, lManifoldBuffer, &lNContactsA, abInA );
		}

//		int firstSet = GET_LOCAL_IDX/32;

		output2_1( &bodyA, &bodyB, 
			iPairAB.x, iPairAB.y, 
			&lNContactsA, lCPointsA, 
			center[0], nA, shapeData, contactsOut, nContactsOut, capacity, collisionMargin );
	}
}

