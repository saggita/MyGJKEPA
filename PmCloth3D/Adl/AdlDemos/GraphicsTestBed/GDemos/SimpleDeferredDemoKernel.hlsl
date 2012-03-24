//	MAX_LIGHTS == 128 : 4KB for ldsLightPos, ldsLightCol
#define MAX_LIGHTS 256
//#define NUM_LIGHTS_PER_PASS 2
#define NUM_LIGHTS_PER_PASS 128
#define g_fov  35.f
#define G_WIDTH 1024
#define G_HEIGHT 720
#define DX (2.f/(float)G_WIDTH)
#define DY (2.f/(float)G_HEIGHT)

#define PI 3.141576f

#define NUM_TILES_PER_CLUSTER 8
#define TILE_SIZE 8
#define CLUSTER_SIZE (NUM_TILES_PER_CLUSTER*TILE_SIZE)
#define MAX_LIGHTS_PER_TILE 128
#define MAX_LIGHTS_PER_TILE_IN_32B (MAX_LIGHTS_PER_TILE/32)


RWStructuredBuffer<uint> rwLightIdxBuffer : register(u0);

[numthreads(64, 1, 1)]
void ClearLightIdxKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	rwLightIdxBuffer[gIdx.x] = 0;
}

cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 m_gData;
}

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

int calcNumTiles(int size, int clusterSize)
{
	return max( 1, (size/clusterSize)+(!(size%clusterSize)?0:1) );
}


static const float2 g_extent[2] = { float2(-1, -1), float2(1, 1) };


StructuredBuffer<float4> lightPos : register(t0);
StructuredBuffer<float4> lightCol : register(t1);
Texture2D colorIn : register( t2 );
Texture2D posIn : register( t3 ); 
Texture2D normalIn : register( t4 ); 
StructuredBuffer<uint> rLightIdxBuffer : register(t5);

RWStructuredBuffer<float4> bufferOut : register(u0);


groupshared Aabb2D ldsAabb[NUM_LIGHTS_PER_PASS];
groupshared float4 ldsLightPos[NUM_LIGHTS_PER_PASS];
groupshared float4 ldsLightCol[NUM_LIGHTS_PER_PASS];
groupshared uint ldsFlg;
groupshared uint ldsLightFlg[MAX_LIGHTS_PER_TILE_IN_32B];
groupshared uint ldsLightCounter;
groupshared uint ldsGrid[NUM_TILES_PER_CLUSTER][NUM_TILES_PER_CLUSTER][MAX_LIGHTS_PER_TILE_IN_32B];
groupshared uint ldsTileLightIdx[NUM_LIGHTS_PER_PASS];
groupshared uint ldsTileLightCounter;

//	todo. better way to get screen space size of sphere?
Aabb2D calcAabb2D(float4 p0)
{
	float p0Length = length( p0.xyz );

	float ssLengthY =  p0.w*1.f/tan(g_fov/2.f*PI/180.f)/p0Length;
	float ssLengthX =  ssLengthY*G_HEIGHT/G_WIDTH;

	float4 sp0 = mul( float4(p0.xyz,1), Projection );
	sp0/=sp0.w;

	float2 r = float2( ssLengthX, ssLengthY );

	Aabb2D aabb;
	aabb.m_min = sp0.xy - r;
	aabb.m_max = sp0.xy + r;
	return aabb;
}

[numthreads(64, 1, 1)]
void BuildLightIdxKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	int iter = 0;
//		for(int iter=0; iter<MAX_LIGHTS/NUM_LIGHTS_PER_PASS; iter++)
	{
		int offset = NUM_LIGHTS_PER_PASS*iter;
		for(int i=lIdx1D; i < NUM_LIGHTS_PER_PASS; i+=TILE_SIZE*TILE_SIZE )
		{
			ldsLightPos[i] = lightPos[i + offset];
			ldsLightCol[i] = lightCol[i + offset];
			float fallOff = ldsLightPos[i].w;
			ldsLightPos[i].w = 1.f;
			ldsLightPos[i] = mul( ldsLightPos[i], View );
			ldsLightPos[i].w = fallOff;

			ldsAabb[i] = calcAabb2D( ldsLightPos[i] );
		}
	}
	GroupMemoryBarrierWithGroupSync();

	int nTilesX = calcNumTiles(G_WIDTH, CLUSTER_SIZE);
	int nTilesY = calcNumTiles(G_HEIGHT, CLUSTER_SIZE);

	//int lightIdx = 0;
	for(int lightIdx=gIdx.x; lightIdx<NUM_LIGHTS_PER_PASS; lightIdx+=64)
	{
		Aabb2D lightAabb = ldsAabb[lightIdx];

		uint2 tileCrdMax = uint2((lightAabb.m_max.x+1.f)*nTilesX/2.f,
			(1.f-lightAabb.m_max.y)*nTilesY/2.f);
		uint2 tileCrdMin = uint2((lightAabb.m_min.x+1.f)*nTilesX/2.f,
			(1.f-lightAabb.m_min.y)*nTilesY/2.f);

		for(int tj=max(tileCrdMax.y,0); tj<min(tileCrdMin.y+1, nTilesY); tj++)
		for(int ti=max(tileCrdMin.x,0); ti<min(tileCrdMax.x+1, nTilesX); ti++)
		{
			uint rgbaIdx = lightIdx/32;
			uint modIdx = lightIdx&(31);

			uint bitFlg = (1<<modIdx);
			uint orig;

			int idx = ti+tj*nTilesX;
			InterlockedAdd( rwLightIdxBuffer[idx*MAX_LIGHTS_PER_TILE_IN_32B + rgbaIdx], bitFlg, orig );
		}
	}
}


//#define USE_LOCAL_GRID 1

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void PostProcessKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	//	load bit flags to LDS
	if( lIdx1D < MAX_LIGHTS_PER_TILE_IN_32B )
	{
		uint2 clusterIdx = uint2(wgIdx.x, wgIdx.y);

		int nTilesX = calcNumTiles(G_WIDTH, CLUSTER_SIZE);

		int idx = clusterIdx.x+clusterIdx.y*nTilesX;

		ldsLightFlg[lIdx1D] = rLightIdxBuffer[idx*MAX_LIGHTS_PER_TILE_IN_32B+lIdx1D];
	}

	GroupMemoryBarrierWithGroupSync();

	//	clear counter
	if( lIdx1D == 0 )
	{
		ldsFlg = 0;
		for(int i=0; i<MAX_LIGHTS_PER_TILE_IN_32B; i++)
		{
			ldsFlg += ldsLightFlg[i];
		}

		ldsLightCounter = 0;
	}

	GroupMemoryBarrierWithGroupSync();

	if( ldsFlg == 0 ) return;

	//	load lights to LDS

//	if( lIdx1D < MAX_LIGHTS_PER_TILE )
	for(int lightIdx=lIdx1D; lightIdx<MAX_LIGHTS_PER_TILE; lightIdx+=TILE_SIZE*TILE_SIZE)
	{
		uint rgbaIdx = lightIdx/32;
		uint modIdx = lightIdx&(31);
		uint bitFlg = (1<<modIdx);

		if( ldsLightFlg[rgbaIdx] & bitFlg )
		{
			uint dst;
			InterlockedAdd( ldsLightCounter, 1, dst );

			float4 lwp = lightPos[lightIdx];
			ldsLightCol[dst] = lightCol[lightIdx];
			float fallOff = lwp.w;
			lwp.w = 1.f;
			lwp = mul( lwp, View );
			lwp.w = fallOff;
			ldsLightPos[dst] = lwp;
#ifdef USE_LOCAL_GRID
			ldsAabb[dst] = calcAabb2D( lwp );
#endif
		}
	}

	GroupMemoryBarrierWithGroupSync();
#ifdef USE_LOCAL_GRID
	//	clear grid
	{
		if( lIdx.x < NUM_TILES_PER_CLUSTER && lIdx.y < NUM_TILES_PER_CLUSTER )
		{
			for(int i=0; i<MAX_LIGHTS_PER_TILE_IN_32B; i++)
				ldsGrid[lIdx.x][lIdx.y][i] = 0;
		}
	}

	GroupMemoryBarrierWithGroupSync();

	//	build grid
	{
		int nTilesX = calcNumTiles(G_WIDTH, CLUSTER_SIZE);
		int nTilesY = calcNumTiles(G_HEIGHT, CLUSTER_SIZE);

		float2 clusterOrig = float2(wgIdx.x*NUM_TILES_PER_CLUSTER, wgIdx.y*NUM_TILES_PER_CLUSTER);

		float2 clusterScale = float2( (nTilesX*NUM_TILES_PER_CLUSTER)/2.f, (nTilesY*NUM_TILES_PER_CLUSTER)/2.f );
		//	todo. iterate through
//		uint lightIdx = lIdx1D;
		for(int lightIdx=lIdx1D; lightIdx<ldsLightCounter; lightIdx+=TILE_SIZE*TILE_SIZE)
//		if( lightIdx < ldsLightCounter )
		{
			Aabb2D lightAabb = ldsAabb[lightIdx];

			uint2 tileCrdMax = uint2((lightAabb.m_max.x+1.f)*clusterScale.x,
				(1.f-lightAabb.m_max.y)*clusterScale.y);
			uint2 tileCrdMin = uint2((lightAabb.m_min.x+1.f)*clusterScale.x,
				(1.f-lightAabb.m_min.y)*clusterScale.y);

			for(int tj=max(tileCrdMax.y,clusterOrig.y); tj<min(tileCrdMin.y+1, clusterOrig.y+NUM_TILES_PER_CLUSTER); tj++)
			for(int ti=max(tileCrdMin.x,clusterOrig.x); ti<min(tileCrdMax.x+1, clusterOrig.x+NUM_TILES_PER_CLUSTER); ti++)
			{
				int ltj = tj-clusterOrig.y;
				int lti = ti-clusterOrig.x;

				uint rgbaIdx = lightIdx/32;
				uint modIdx = lightIdx&(31);

				uint bitFlg = (1<<modIdx);
				uint orig;

//				if( ltj<NUM_TILES_PER_CLUSTER && lti<NUM_TILES_PER_CLUSTER )
				InterlockedAdd( ldsGrid[lti][ltj][rgbaIdx], bitFlg, orig );
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();
#endif
	//	process tiles one by one
	for(int tileIdx=0; tileIdx<NUM_TILES_PER_CLUSTER*NUM_TILES_PER_CLUSTER; tileIdx++)
	{
		int ti = tileIdx%NUM_TILES_PER_CLUSTER;
		int tj = tileIdx/NUM_TILES_PER_CLUSTER;

		uint2 tIdx = uint2(wgIdx.x*CLUSTER_SIZE + ti*TILE_SIZE, wgIdx.y*CLUSTER_SIZE + tj*TILE_SIZE);
		uint xIdx = lIdx.x + tIdx.x;
		uint yIdx = lIdx.y + tIdx.y;

		float4 c = colorIn.Load( uint3(xIdx,yIdx,0) );
		float4 p = posIn.Load( uint3(xIdx,yIdx,0) );
		float4 n = normalIn.Load( uint3(xIdx,yIdx,0) );

		float4 ans = float4(0,0,0,0);
		{
			float s = 0.15f;
			ans = float4(s,s,s,0);
		}
#ifdef USE_LOCAL_GRID
		if( lIdx1D == 0 )
		{
			uint sum = 0;
			for(int i=0; i<MAX_LIGHTS_PER_TILE_IN_32B; i++)
			{
				sum += ldsGrid[ti][tj][i];
			}
			ldsFlg = sum;
		}
		GroupMemoryBarrierWithGroupSync();


		if( ldsFlg )
		{	//	gather light data
/*
			if( lIdx1D == 0 )
			{
				ldsTileLightCounter = 0;
			}
			GroupMemoryBarrierWithGroupSync();
			if( lIdx1D < MAX_LIGHTS_PER_TILE )
			{
				uint rgbaIdx = lIdx1D/32;
				uint modIdx = lIdx1D&(31);
				uint bitFlg = (1<<modIdx);

				if( ldsGrid[ti][tj][rgbaIdx] & bitFlg )
				{
					uint dst;
					InterlockedAdd( ldsTileLightCounter, 1, dst );
					ldsTileLightIdx[dst] = lIdx1D;
				}
			}
			GroupMemoryBarrierWithGroupSync();

			//	do the lighting

			float4 vVec = float4(-normalize(p.xyz),0);
			for(int ie=0; ie<ldsTileLightCounter; ie++)
			{
				int i = ldsTileLightIdx[ie];
				float4 lp = ldsLightPos[i];
				float4 lc = ldsLightCol[i];
				float fallOff2 = lp.w*lp.w;

				float4 lVec = lp - p;
				float lVecLength2 = dot( lVec.xyz, lVec.xyz );

				if( lVecLength2 < fallOff2 )
				{
					float s = max(0.f, dot(n.xyz, lVec.xyz));
					s *= 1.f-lVecLength2/fallOff2;
					ans += s*lc*0.9f;
				}
			}
*/
			float4 vVec = float4(-normalize(p.xyz),0);
			for(int i=0; i<ldsLightCounter; i++)
			{
				float4 lp = ldsLightPos[i];
				float4 lc = ldsLightCol[i];
				float fallOff2 = lp.w*lp.w;

				float4 lVec = lp - p;
				float lVecLength2 = dot( lVec.xyz, lVec.xyz );

				if( lVecLength2 < fallOff2 )
				{
					float s = max(0.f, dot(n.xyz, lVec.xyz));
					s *= 1.f-lVecLength2/fallOff2;
					ans += s*lc*0.9f;
				}
			}

		}
#else
		{
			float4 vVec = float4(-normalize(p.xyz),0);
			for(int i=0; i<ldsLightCounter; i++)
			{
				float4 lp = ldsLightPos[i];
				float4 lc = ldsLightCol[i];
				float fallOff2 = lp.w*lp.w;
				float fallOff = lp.w;

				float4 lVec = lp - p;
				float lVecLength2 = dot( lVec.xyz, lVec.xyz );

				if( lVecLength2 < fallOff2 )
				{
					float coeff = 1.f-sqrt(lVecLength2)/fallOff;
					float s = max(0.f, dot(n.xyz, lVec.xyz));
					s *= coeff;
					ans += s*lc*1.3f;

					//	todo. how to fall off specular??
					bool specular = true;
					if(specular)
					{
						lVec.xyz = normalize(lVec.xyz);
						float4 hVec = float4( normalize(lVec.xyz + vVec.xyz), 0 );

						float s = pow( max(dot(n.xyz,hVec.xyz), 0.f), 100.f );
						s *= coeff;
						ans += s*lc*8.f;
					}
				}

			}
		}
#endif
		bufferOut[xIdx + yIdx*G_WIDTH] = ans*c;
	}


/*
	if( ldsLightCounter && lIdx1D == 0 )
	{
		float2 clusterOrig = float2(wgIdx.x*NUM_TILES_PER_CLUSTER, wgIdx.y*NUM_TILES_PER_CLUSTER);

		int lightIdx = 0;
		{
			Aabb2D lightAabb = ldsAabb[lightIdx];

			for(int tj=0; tj<NUM_TILES_PER_CLUSTER; tj++)
			for(int ti=0; ti<NUM_TILES_PER_CLUSTER; ti++)
			{
				int x = (clusterOrig.x + ti) * TILE_SIZE;
				int y = (clusterOrig.y + tj) * TILE_SIZE;

				if( ldsGrid[ti][tj][0] )
				{
					bufferOut[x + y*G_WIDTH] = float4(1,1,1,1);
				}
			}
		}
	}
*/
	//=================
	//=================

/*
	int iter = 0;
//		for(int iter=0; iter<MAX_LIGHTS/NUM_LIGHTS_PER_PASS; iter++)
	{
		int offset = NUM_LIGHTS_PER_PASS*iter;
		for(int i=lIdx1D; i < NUM_LIGHTS_PER_PASS; i+=TILE_SIZE*TILE_SIZE )
		{
			ldsLightPos[i] = lightPos[i + offset];
			ldsLightCol[i] = lightCol[i + offset];
			float fallOff = ldsLightPos[i].w;
			ldsLightPos[i].w = 1.f;
			ldsLightPos[i] = mul( ldsLightPos[i], View );
			ldsLightPos[i].w = fallOff;

#ifdef TILE_CULLING
			ldsAabb[i] = calcAabb2D( ldsLightPos[i] );
#endif
		}
	}
	GroupMemoryBarrierWithGroupSync();

	for(int tileIdx=0; tileIdx<NUM_TILES_PER_CLUSTER*NUM_TILES_PER_CLUSTER; tileIdx++)
	{
		int ti = tileIdx%NUM_TILES_PER_CLUSTER;
		int tj = tileIdx/NUM_TILES_PER_CLUSTER;

		uint2 tIdx = uint2(wgIdx.x*CLUSTER_SIZE + ti*TILE_SIZE, wgIdx.y*CLUSTER_SIZE + tj*TILE_SIZE);
		uint xIdx = lIdx.x + tIdx.x;
		uint yIdx = lIdx.y + tIdx.y;

		float4 c = colorIn.Load( uint3(xIdx,yIdx,0) );
		float4 p = posIn.Load( uint3(xIdx,yIdx,0) );
		float4 n = normalIn.Load( uint3(xIdx,yIdx,0) );

		float4 ans = float4(0,0,0,0);
		{
			float s = 0.05f;
			ans = float4(s,s,s,0);
		}

#ifdef TILE_CULLING
		Aabb2D tileAabb;
		tileAabb.m_min = float2( (float)tIdx.x*DX-1.f, 1.f-(float)tIdx.y*DY );
		tileAabb.m_max = float2( (float)(tIdx.x+TILE_SIZE-1)*DX-1.f, 
			1.f-(float)(tIdx.y+TILE_SIZE-1)*DY );
#endif
		{
			float4 vVec = float4(-normalize(p.xyz),0);
//			int i = 0;
			for(int i=0; i<NUM_LIGHTS_PER_PASS; i++)
//			for(int i=0; i<5; i++)
			{
#ifdef TILE_CULLING
				if(0)
				{
					if( lIdx1D == 0 )
					{
						ldsTileOvl = Aabb2DOverlaps( tileAabb, ldsAabb[i] );
					}
					GroupMemoryBarrierWithGroupSync();

					if( !ldsTileOvl ) continue;
				}
#endif
				float4 lp = ldsLightPos[i];
				float4 lc = ldsLightCol[i];
				float fallOff2 = lp.w*lp.w;

				float4 lVec = lp - p;
				float lVecLength2 = dot( lVec.xyz, lVec.xyz );


				if( lVecLength2 < fallOff2 )
				{
					float s = max(0.f, dot(n.xyz, lVec.xyz));
					s *= 1.f-lVecLength2/fallOff2;
					ans += s*lc*0.9f;
				}

#ifdef TILE_CULLING
//				if(0)
				{
					if( Aabb2DOverlaps( tileAabb, ldsAabb[i] ) )
					{
						ans += 0.1f*(float)tileIdx/(float)(NUM_TILES_PER_CLUSTER*NUM_TILES_PER_CLUSTER)
							*lc;
					}
				}
#endif
			}
		}

		{
			uint2 clusterIdx = uint2(wgIdx.x, wgIdx.y);

			int nTilesX = calcNumTiles(G_WIDTH, CLUSTER_SIZE);
			int nTilesY = calcNumTiles(G_HEIGHT, CLUSTER_SIZE);

			int idx = clusterIdx.x+clusterIdx.y*nTilesX;

			uint lightFlg = rLightIdxBuffer[idx*MAX_LIGHTS_PER_TILE_IN_32B];

			for(int i=0; i<NUM_LIGHTS_PER_PASS; i++)
			{
				if( lightFlg & (1<<i))
				{
					ans = ldsLightCol[i];
				}
			}

		}

		if( xIdx < G_WIDTH && yIdx < G_HEIGHT )
			bufferOut[xIdx + yIdx*G_WIDTH] = ans*c;


		if(0)
		{
//			int tileIdx = ti+tj*NUM_TILES_PER_CLUSTER;
			bufferOut[xIdx + yIdx*G_WIDTH].x += 0.3f*(float)tileIdx/(float)(NUM_TILES_PER_CLUSTER*NUM_TILES_PER_CLUSTER);
		}
	}
*/
}

