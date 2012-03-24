cbuffer CB0 : register( b0 )
{
	matrix m_view;
	matrix m_projection;
	float m_maxDepth;
	int m_fullWidth;
	int m_lowResWidth;
	int m_lowResHeight;
	int m_nBodies;
	int m_maxOvlPerObj;
};


Texture2D colorIn : register( t0 ); 
StructuredBuffer<float4> bbsBuffer : register(t0);
StructuredBuffer<uint> lowResBuffer : register(t1);

RWStructuredBuffer<float4> bufferOut : register(u0);
RWStructuredBuffer<uint> lowResBufferOut : register(u1);
RWStructuredBuffer<uint> visibleIdxOut : register(u2);

#define g_fov  35.f
#define G_WIDTH 1024
#define G_HEIGHT 720
#define PI 3.141576f

#define FLT_MAX         3.402823466e+38F
#define CELL_RES 16

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

Aabb2D calcAabb2D(float4 p0)
{
	float p0Length = length( p0.xyz );

	float ssLengthY =  p0.w*1.f/tan(g_fov/2.f*PI/180.f)/p0Length;
	float ssLengthX =  ssLengthY*G_HEIGHT/G_WIDTH;

	float4 sp0 = mul( float4(p0.xyz,1), m_projection );
	sp0/=sp0.w;

	float2 r = float2( ssLengthX, ssLengthY );

	Aabb2D aabb;
	aabb.m_min = sp0.xy - r;
	aabb.m_max = sp0.xy + r;
	return aabb;
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

//=============================
//=============================
//=============================

Aabb2D projCrdToGridCrd(Aabb2D aabb)
{
	uint2 gridCrdMin = uint2((aabb.m_min.x+1.f)*m_lowResWidth/2.f,
		(aabb.m_min.y+1.f)*m_lowResHeight/2.f);
	uint2 gridCrdMax = uint2((aabb.m_max.x+1.f)*m_lowResWidth/2.f,
		(aabb.m_max.y+1.f)*m_lowResHeight/2.f);

	Aabb2D aabbOut;
	aabbOut.m_min = uint2(max(gridCrdMin.x,0), max(gridCrdMin.y,0));
	aabbOut.m_max = uint2(min(gridCrdMax.x, m_lowResWidth), min(gridCrdMax.y, m_lowResHeight));
	return aabbOut;
}

Aabb2D gridCrdToCellCrd(Aabb2D aabb)
{
	uint2 gridCrdMin = aabb.m_min;
	uint2 gridCrdMax = aabb.m_max;

	Aabb2D aabbOut;
//	aabbOut.m_min = uint2( (gridCrdMin.x+CELL_RES-1)/CELL_RES, (gridCrdMin.y+CELL_RES-1)/CELL_RES );
//	aabbOut.m_max = uint2( (gridCrdMax.x+CELL_RES-1)/CELL_RES, (gridCrdMax.y+CELL_RES-1)/CELL_RES );
	aabbOut.m_min = uint2( (gridCrdMin.x)/CELL_RES, (gridCrdMin.y)/CELL_RES );
	aabbOut.m_max = uint2( (gridCrdMax.x)/CELL_RES, (gridCrdMax.y)/CELL_RES );
	return aabbOut;
}

int getNCellsX()
{
	return (m_lowResWidth+CELL_RES-1)/CELL_RES;
}










//=============================
//=============================
//=============================

u32 viewZtoUintZ(float viewZ)
{
	return viewZ/m_maxDepth*0xffff;
}

float uintZtoViewZ(u32 z)
{
	return z/(float)0xffff*m_maxDepth;
}


[numthreads(8, 8, 1)]
void PostProcessKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	float4 col = colorIn.Load( uint3(gIdx.x,gIdx.y,0) );
	float depth = colorIn.Load( uint3(gIdx.x,gIdx.y,0) ).w/m_maxDepth;

	bufferOut[gIdx.x + gIdx.y*m_fullWidth] = depth;
	bufferOut[gIdx.x + gIdx.y*m_fullWidth] = col*(1.f-depth);
}


groupshared uint ldsMax;

[numthreads(8, 8, 1)]
void ZReduceKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	uint depth = viewZtoUintZ( colorIn.Load( uint3(gIdx.x,gIdx.y,0) ).w );

	if( lIdx1D == 0 )
	{
		ldsMax = depth;
	}

	GroupMemoryBarrierWithGroupSync();

	uint oldValue;
	InterlockedMax( ldsMax, depth, oldValue );

	GroupMemoryBarrierWithGroupSync();

	if( lIdx1D == 0 )
		lowResBufferOut[wgIdx.x + wgIdx.y*m_lowResWidth ] = ldsMax;
}



[numthreads(8, 8, 1)]
void UpScaleKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	float4 value = lowResBufferOut[wgIdx.x+wgIdx.y*m_lowResWidth]/(float)0xffff;

	if( wgIdx.x % CELL_RES == 0 || wgIdx.y % CELL_RES == 0 ) value *= float4(0.9f,0,0,1);

	bufferOut[gIdx.x + gIdx.y*m_fullWidth] = value;
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

[numthreads(64, 1, 1)]
void QueryKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	if( gIdx.x >= m_nBodies ) return;

	{
		uint myZ;
		float4 bSphere = bbsBuffer[ gIdx.x ];
		float rad = bSphere.w;
		bSphere.w = 1.f;
		bSphere = mul( bSphere, m_view );

		if( bSphere.z < rad ) return;

		myZ = viewZtoUintZ(bSphere.z-rad);

		Aabb2D aabb = calcSSAabb(bSphere, rad);

		//	todo. do frustum culling before occlusion culling
		if( aabb.m_max.x < -1.f || aabb.m_min.x > 1.f ) return;
		if( aabb.m_max.y < -1.f || aabb.m_min.y > 1.f ) return;

		Aabb2D gridCrd = projCrdToGridCrd( aabb );

		uint visible = 0;
		for(int tj=gridCrd.m_min.y; tj<=gridCrd.m_max.y; tj++)
		for(int ti=gridCrd.m_min.x; ti<=gridCrd.m_max.x; ti++)
		{
			uint z = lowResBufferOut[ti + tj*m_lowResWidth];
			visible |= ( myZ <= z );
		}

		visibleIdxOut[ gIdx.x ] = (visible)? 1: 0;
	}
}

//StructuredBuffer<float4> bbsBuffer : register(t0);
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
			if( aabb.m_max.x < -1.f || aabb.m_min.x > 1.f ) return;
			if( aabb.m_max.y < -1.f || aabb.m_min.y > 1.f ) return;

			Aabb2D gridCrd = projCrdToGridCrd( aabb );
			cellCrd = gridCrdToCellCrd( gridCrd );

			nX = (cellCrd.m_max.x-cellCrd.m_min.x)+1;
			nY = (cellCrd.m_max.y-cellCrd.m_min.y)+1;

//			count = (cellCrd.m_max.x-cellCrd.m_min.x)*(cellCrd.m_max.y-cellCrd.m_min.y);
			count = nX*nY;

			nX = cellCrd.m_min.x;
			nY = cellCrd.m_max.x;

			nZ = gridCrd.m_min.x;
			nW = gridCrd.m_max.x;
		}
	}

	visibleIdxOutCNO[gIdx.x] = (count>m_maxOvlPerObj)? 1:0;
if(0)
	if( cellCrd.m_max.x == 5 && cellCrd.m_max.y == 1 )
	{
		if( gIdx.x == 1750 )
			visibleIdxOutCNO[gIdx.x] = (1<<2);
	}

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

StructuredBuffer<u32> lowerBoundCBT : register(t0);
StructuredBuffer<u32> upperBoundCBT : register(t1);
StructuredBuffer<SortData> sortDataCBT : register(t2);
StructuredBuffer<float4> bbsBufferCBT : register(t3);
StructuredBuffer<uint> lowResBufferCBT : register(t4);

RWStructuredBuffer<uint> visibleIdxOutCBT : register(u0);


groupshared uint ldsDepth[CELL_RES*CELL_RES];


[numthreads(CELL_RES, CELL_RES, 1)]
void CullByTileKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	int nCellsX = getNCellsX();

	int cellIdxX = wgIdx.x;
	int cellIdxY = wgIdx.y;

	int cellIdx = wgIdx.x + wgIdx.y*nCellsX;

//	if( wgIdx.x != 5 || wgIdx.y != 1 ) return;
	int xOffset = CELL_RES*cellIdxX;
	int yOffset = CELL_RES*cellIdxY;
	{
		ldsDepth[lIdx.x+lIdx.y*CELL_RES] = lowResBufferCBT[xOffset+lIdx.x + (yOffset+lIdx.y)*m_lowResWidth];
	}

	GroupMemoryBarrierWithGroupSync();

//	if( lIdx1D != 0 ) return;

	for(int ie=lowerBoundCBT[cellIdx]+lIdx1D; ie<upperBoundCBT[cellIdx]; ie+=CELL_RES*CELL_RES)
	{
		SortData iData = sortDataCBT[ie];
		int idx = iData.m_value;

		uint myZ;
		float4 bSphere = bbsBufferCBT[ idx ];
		float rad = bSphere.w;
		bSphere.w = 1.f;
		bSphere = mul( bSphere, m_view );

		if( bSphere.z < rad ) return;

		myZ = viewZtoUintZ(bSphere.z-rad);

		Aabb2D aabb = calcSSAabb(bSphere, rad);

		Aabb2D gridCrd = projCrdToGridCrd( aabb );

		{
			gridCrd.m_min.x = max( 0, gridCrd.m_min.x-xOffset );
			gridCrd.m_min.y = max( 0, gridCrd.m_min.y-yOffset );
			gridCrd.m_max.x = min( CELL_RES-1, gridCrd.m_max.x-xOffset );
			gridCrd.m_max.y = min( CELL_RES-1, gridCrd.m_max.y-yOffset );

			uint visible = 0;
			for(int tj=gridCrd.m_min.y; tj<=gridCrd.m_max.y; tj++)
			for(int ti=gridCrd.m_min.x; ti<=gridCrd.m_max.x; ti++)
			{
				int iIdx = ti;
				int jIdx = tj;
				uint z = ldsDepth[iIdx + jIdx*CELL_RES];
	//			visible |= ( myZ <= z );
				if( myZ <= z ) visible = (1<<1);
			}
			InterlockedOr( visibleIdxOutCBT[idx], visible );
		}
	}
}


/*
[numthreads(16, 16, 1)]
void CullByTileKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	int nCellsX = getNCellsX();

	int cellIdxX = wgIdx.x;
	int cellIdxY = wgIdx.y;

	int cellIdx = wgIdx.x + wgIdx.y*nCellsX;

//	if( wgIdx.x != 5 || wgIdx.y != 1 ) return;

	if( lIdx1D != 0 ) return;

	for(int ie=lowerBoundCBT[cellIdx]; ie<upperBoundCBT[cellIdx]; ie++)
	{
		SortData iData = sortDataCBT[ie];
		int idx = iData.m_value;

		uint myZ;
		float4 bSphere = bbsBufferCBT[ idx ];
		float rad = bSphere.w;
		bSphere.w = 1.f;
		bSphere = mul( bSphere, m_view );

		if( bSphere.z < rad ) return;

		myZ = (bSphere.z-rad)/m_maxDepth*0xffff;

		Aabb2D aabb = calcSSAabb(bSphere, rad);

		Aabb2D gridCrd = projCrdToGridCrd( aabb );


		if(1)
		{
			int xOffset = CELL_RES*cellIdxX;
			int yOffset = CELL_RES*cellIdxY;
			gridCrd.m_min.x = max( 0, gridCrd.m_min.x-xOffset );
			gridCrd.m_min.y = max( 0, gridCrd.m_min.y-yOffset );
			gridCrd.m_max.x = min( CELL_RES-1, gridCrd.m_max.x-xOffset );
			gridCrd.m_max.y = min( CELL_RES-1, gridCrd.m_max.y-yOffset );

			uint visible = 0;
			for(int tj=gridCrd.m_min.y; tj<=gridCrd.m_max.y; tj++)
			for(int ti=gridCrd.m_min.x; ti<=gridCrd.m_max.x; ti++)
			{
				int iIdx = ti+xOffset;
				int jIdx = tj+yOffset;
				uint z = lowResBufferCBT[iIdx + jIdx*m_lowResWidth];
	//			visible |= ( myZ <= z );
				if( myZ <= z ) visible = (1<<1);
			}
			InterlockedOr( visibleIdxOutCBT[idx], visible );
		}
		else
		{
			uint visible = 0;
			for(int tj=gridCrd.m_min.y; tj<=gridCrd.m_max.y; tj++)
			for(int ti=gridCrd.m_min.x; ti<=gridCrd.m_max.x; ti++)
			{
				int iIdx = ti;
				int jIdx = tj;
				uint z = lowResBufferCBT[iIdx + jIdx*m_lowResWidth];
				if( myZ <= z ) visible = (1<<1);
			}
			InterlockedOr( visibleIdxOutCBT[idx], visible );

//			if( cellIdxX == 5 && cellIdxY == 1 ) if( visible == 0 ) visibleIdxOutCBT[idx] = (1<<2);
		}

	}
}
*/
