#define FLT_MAX         3.402823466e+38F

#define DEFAULT_ARGS uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID
#define GET_GROUP_IDX groupIdx.x
#define GET_LOCAL_IDX localIdx.x
#define GET_GLOBAL_IDX globalIdx.x
#define GROUP_LDS_BARRIER GroupMemoryBarrierWithGroupSync()


cbuffer CB0 : register( b0 )
{
	matrix m_view;
	matrix m_projection;
	matrix m_projectionInv;
	int m_width;
	int m_height;
	int m_nBodies;
	int m_maxOvlPerObj;
	int m_tileRes;
};


#define u32 uint

typedef struct
{
	u32 m_key;
	u32 m_value;
}SortData;


typedef struct
{
	float2 m_min;
	float2 m_max;
}Aabb2D;

bool Aabb2DOverlaps( const Aabb2D a, const Aabb2D b )
{
	if( a.m_max.x < b.m_min.x || a.m_min.x > b.m_max.x ) return false;
	if( a.m_max.y < b.m_min.y || a.m_min.y > b.m_max.y ) return false;

	return true;
}

Aabb2D Aabb2DSetEmpty()
{
	Aabb2D aabb;
	aabb.m_min = float2( FLT_MAX, FLT_MAX );
	aabb.m_max = -float2( FLT_MAX, FLT_MAX );
	return aabb;
}

Aabb2D Aabb2DIncludePoint( Aabb2D aabb, float2 p )
{
	Aabb2D aabbOut;
	aabbOut.m_max = max( aabb.m_max, p );
	aabbOut.m_min = min( aabb.m_min, p );
	return aabbOut;
}

static const float3 g_positions[8] = { float3(-1, -1, -1), float3(1, -1, -1), float3(1, 1, -1), float3(-1, 1, -1), 
										float3(-1, -1, 1), float3(1, -1, 1), float3(1, 1, 1), float3(-1, 1, 1) };


float2 calcProjectionCrd( float4 v, matrix proj )
{
	float4 p = mul( v, proj );
	return p.xy/p.w;
}

Aabb2D calcSSAabb(float4 pos, float rad)
{
	Aabb2D aabb = Aabb2DSetEmpty();
	for(int i=0; i<8; i++)
	{
		aabb = Aabb2DIncludePoint( aabb, calcProjectionCrd( pos + rad*float4(g_positions[i],0), m_projection ) );
	}

	{	//	flip Y
		float tmp = aabb.m_min.y;
		aabb.m_min.y = -aabb.m_max.y;
		aabb.m_max.y = -tmp;
	}
	return aabb;
}

//=============================
//=============================
//=============================

Aabb2D projCrdToGridCrd(Aabb2D aabb)
{
	uint2 gridCrdMin = uint2((aabb.m_min.x+1.f)*m_width/2.f,
		(aabb.m_min.y+1.f)*m_height/2.f);
	uint2 gridCrdMax = uint2((aabb.m_max.x+1.f)*m_width/2.f,
		(aabb.m_max.y+1.f)*m_height/2.f);

	Aabb2D aabbOut;
	aabbOut.m_min = uint2(gridCrdMin.x, gridCrdMin.y);
	aabbOut.m_max = uint2(gridCrdMax.x, gridCrdMax.y);
	return aabbOut;
}

int getNCellsX()
{
	return (m_width+m_tileRes-1)/m_tileRes;
}

int getNCellsY()
{
	return (m_height+m_tileRes-1)/m_tileRes;
}

Aabb2D gridCrdToCellCrd(Aabb2D aabb)
{
	uint2 gridCrdMin = aabb.m_min;
	uint2 gridCrdMax = aabb.m_max;

	Aabb2D aabbOut;
//	aabbOut.m_min = uint2( (gridCrdMin.x+m_tileRes-1)/m_tileRes, (gridCrdMin.y+m_tileRes-1)/m_tileRes );
//	aabbOut.m_max = uint2( (gridCrdMax.x+m_tileRes-1)/m_tileRes, (gridCrdMax.y+m_tileRes-1)/m_tileRes );
	aabbOut.m_min = uint2( (gridCrdMin.x)/m_tileRes, (gridCrdMin.y)/m_tileRes );
	aabbOut.m_max = uint2( (gridCrdMax.x)/m_tileRes, (gridCrdMax.y)/m_tileRes );

	aabbOut.m_min = uint2( max( aabbOut.m_min.x, 0 ), max( aabbOut.m_min.y, 0 ) );
	aabbOut.m_max = uint2( min( aabbOut.m_max.x, getNCellsX()-1 ), min( aabbOut.m_max.y, getNCellsY()-1 ) );
	return aabbOut;
}


//--------------------------------------
//--------------------------------------
//--------------------------------------
//--------------------------------------


StructuredBuffer<float4> bbsBuffer : register(t0);
RWStructuredBuffer<uint> countOut : register(u0);
RWStructuredBuffer<uint> visibleIdxOutCNO : register(u1);

[numthreads(64, 1, 1)]
void CountNOverlapCellsKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	if( gIdx.x >= m_nBodies ) return;

	int count = 0;
	int nX, nY, nZ, nW;
	Aabb2D cellCrd;
	{
		uint myZ;
		float4 bSphere = bbsBuffer[ gIdx.x ];
		float rad = bSphere.w;
		bSphere.w = 1.f;
		bSphere = mul( bSphere, m_view );

		if( !(bSphere.z < rad) )
		{
			Aabb2D aabb = calcSSAabb(bSphere, rad);

			//	todo. do frustum culling before occlusion culling
			if( aabb.m_max.x < -1.f || aabb.m_min.x > 1.f || aabb.m_max.y < -1.f || aabb.m_min.y > 1.f )
			{
				count = 0;
			}
			else
			{
				Aabb2D gridCrd = projCrdToGridCrd( aabb );
				cellCrd = gridCrdToCellCrd( gridCrd );

				nX = (cellCrd.m_max.x-cellCrd.m_min.x)+1;
				nY = (cellCrd.m_max.y-cellCrd.m_min.y)+1;

				count = nX*nY;

				nX = cellCrd.m_min.x;
				nY = cellCrd.m_max.x;

				nZ = gridCrd.m_min.x;
				nW = gridCrd.m_max.x;
			}
		}
	}

	visibleIdxOutCNO[gIdx.x] = (count>m_maxOvlPerObj)? 1:0;

//	if( gIdx.x == 908 )visibleIdxOutCNO[gIdx.x] = nX | (nY<<8) | (nW<<16);// | (nW<<24);
	countOut[gIdx.x] = (count>m_maxOvlPerObj)? 0:count;
}

//StructuredBuffer<float4> bbsBuffer : register(t0);
StructuredBuffer<u32> sortDataOffset : register(t1);
RWStructuredBuffer<SortData> sortDataOut : register(u0);

[numthreads(64, 1, 1)]
void FillSortDataKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{	//	almost the same to CountNOverlapCellsKernel
	if( gIdx.x >= m_nBodies ) return;

	int count = 0;
	{
		uint myZ;
		float4 bSphere = bbsBuffer[ gIdx.x ];
		float rad = bSphere.w;
		bSphere.w = 1.f;
		bSphere = mul( bSphere, m_view );

		if( !(bSphere.z < rad) )
		{
			Aabb2D aabb = calcSSAabb(bSphere, rad);

			//	todo. do frustum culling before occlusion culling
			if( aabb.m_max.x < -1.f || aabb.m_min.x > 1.f ) return;
			if( aabb.m_max.y < -1.f || aabb.m_min.y > 1.f ) return;

			Aabb2D gridCrd = projCrdToGridCrd( aabb );
			Aabb2D cellCrd = gridCrdToCellCrd( gridCrd );

			count = (cellCrd.m_max.x-cellCrd.m_min.x+1)*(cellCrd.m_max.y-cellCrd.m_min.y+1);

			if( count>m_maxOvlPerObj ) return;

			u32 offset = sortDataOffset[ gIdx.x ];

			int nCellsX = getNCellsX();

			int idx = 0;
			for(int j=cellCrd.m_min.y; j<=cellCrd.m_max.y; j++) for(int i=cellCrd.m_min.x; i<=cellCrd.m_max.x; i++)
			{
				SortData data;
				data.m_key = i+j*nCellsX;
				data.m_value = gIdx.x;
				sortDataOut[ offset+idx ] = data;
				idx++;				
			}
		}
	}
}


#define MAX_LIGHTS_PER_TILE 32


float4 projToView(float4 p)
{
	p = mul( p, m_projectionInv );
	p /= p.w;
	return p;
}


RWStructuredBuffer<uint> lightBitsOut : register(u0);
StructuredBuffer<u32> lowerBound : register(t0);
StructuredBuffer<u32> upperBound : register(t1);
StructuredBuffer<int2> lightIndexBuffer :register(t2);
StructuredBuffer<float4> lightBuffer : register(t3);
Texture2D depthIn : register( t4 );


groupshared float4 ldsLights[MAX_LIGHTS_PER_TILE];



[numthreads(64, 1, 1)]
void PixelLightCullingKernel( DEFAULT_ARGS )
{
	int wgIdx = GET_GROUP_IDX;
	int lIdx = GET_LOCAL_IDX;

	uint tIdx = wgIdx;
	uint start = lowerBound[ tIdx ];
	uint end = upperBound[ tIdx ];
	uint n = end - start;

	for(int ii=lIdx; ii<min(n, MAX_LIGHTS_PER_TILE); ii+=64)
	{
		int idx = lightIndexBuffer[start+ii].y;
		float4 light = lightBuffer[idx];
		{	//	transform to view space
			float fallOff = light.w;
			light.w = 1.f;
			light = mul( light, m_view );
			light.w = fallOff;
		}
		ldsLights[ii] = light;
	}

	GROUP_LDS_BARRIER;

	for(int ii=lIdx; ii<m_tileRes*m_tileRes; ii+=64)
	{
		uint xIdx = (tIdx%getNCellsX()) * m_tileRes + (ii%m_tileRes);
		uint yIdx = (tIdx/getNCellsX()) * m_tileRes + (ii/m_tileRes);
		uint pixelIdx = xIdx+yIdx*m_width;

		uint lightBits = 0;

		float depth = depthIn.Load( uint3(xIdx,yIdx,0) );

		float4 viewPos;
		{	//	restore view position
			float4 projCrd = float4(xIdx/(float)m_width*2.f-1.f, (m_height-yIdx)/(float)m_height*2.f-1.f, depth, 1.f);
			viewPos = projToView( projCrd );
		}

		{
			for(int i=0; i<end-start; i++)
			{
				float4 light = ldsLights[i];
				{
					float4 r = light - viewPos;
					float dist2 = dot(r.xyz, r.xyz);

					if( dist2 < light.w*light.w )
					{
						lightBits |= (1<<i);
					}
				}
			}
		}

		lightBitsOut[pixelIdx] = lightBits;
	}
}

