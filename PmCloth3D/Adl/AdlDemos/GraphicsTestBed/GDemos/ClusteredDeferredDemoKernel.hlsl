//	MAX_LIGHTS == 128 : 4KB for ldsLightPos, ldsLightCol
#define MAX_LIGHTS 4096
//#define NUM_LIGHTS_PER_PASS 2
#define NUM_LIGHTS_PER_PASS 128
#define g_fov  35.f
#define G_WIDTH 1024
#define G_HEIGHT 720
#define DX (2.f/(float)G_WIDTH)
#define DY (2.f/(float)G_HEIGHT)

#define PI 3.141576f

#define NUM_TILES_PER_CLUSTER 1
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
StructuredBuffer<float4> lightCPos : register(t5);

RWStructuredBuffer<float4> bufferOut : register(u0);


groupshared Aabb2D ldsAabb[NUM_LIGHTS_PER_PASS];
groupshared uint ldsLightCIdx[MAX_LIGHTS/NUM_LIGHTS_PER_PASS];
groupshared uint ldsLightClusterCounter;

groupshared float4 ldsLightPos[NUM_LIGHTS_PER_PASS];
groupshared float4 ldsLightCol[NUM_LIGHTS_PER_PASS];
groupshared uint ldsFlg;
groupshared uint ldsLightFlg[MAX_LIGHTS_PER_TILE_IN_32B];
groupshared uint ldsLightCounter;
groupshared uint ldsGrid[NUM_TILES_PER_CLUSTER][NUM_TILES_PER_CLUSTER][MAX_LIGHTS_PER_TILE_IN_32B];

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

float4 calcLighting(float4 vVec, float4 pPos, float4 n, float4 lPos, float4 lColor)
{
	float dCoeff = 0.1f;
	float sCoeff = 0.6f;

	float4 ans = float4(0,0,0,0);

	float fallOff2 = lPos.w*lPos.w;
	float fallOff = lPos.w;

	float4 lVec = lPos - pPos;
	float lVecLength2 = dot( lVec.xyz, lVec.xyz );
	lVec.xyz = normalize(lVec.xyz);

	{
		float coeff = ( lVecLength2 < fallOff2 )? 1.f-sqrt(lVecLength2)/fallOff : 0.f;
		float s = max(0.f, dot(n.xyz, lVec.xyz));
		s *= coeff;
		ans += s*lColor*dCoeff;

		//	todo. how to fall off specular??
		bool specular = true;
		if(specular)
		{
			float4 hVec = float4( normalize(lVec.xyz + vVec.xyz), 0 );

			float s = pow( max(dot(n.xyz,hVec.xyz), 0.f), 100.f );
			s *= coeff;
			ans += s*lColor*sCoeff;
		}
	}
	return ans;
}


float4 calcLighting4(float4 vVec, float4 pPos, float4 n, float4 lPos[4], float4 lColor[4])
{
	float dCoeff = 0.1f;
	float sCoeff = 0.6f;

	float4 ans = float4(0,0,0,0);
	float fallOff2[4];
	float fallOff[4];
	float4 lVec[4];
	float lVecLength2[4];

	for(int ii=0; ii<4; ii++)
	{
		fallOff2[ii] = lPos[ii].w*lPos[ii].w;
		fallOff[ii] = lPos[ii].w;
		lVec[ii] = lPos[ii] - pPos;
		lVecLength2[ii] = dot( lVec[ii].xyz, lVec[ii].xyz );
		lVec[ii].xyz = normalize(lVec[ii].xyz);
	}

	{
		float coeff[4];

		for(int ii=0; ii<4; ii++)
		{
			coeff[ii] = ( lVecLength2[ii] < fallOff2[ii] )? 1.f-sqrt(lVecLength2[ii])/fallOff[ii] : 0.f;
			float s = max(0.f, dot(n.xyz, lVec[ii].xyz));
			s *= coeff[ii];
			ans += s*lColor[ii]*dCoeff;
		}

		//	todo. how to fall off specular??
		bool specular = true;

		for(int ii=0; ii<4; ii++)
		{
			if(specular)
			{
				float4 hVec = float4( normalize(lVec[ii].xyz + vVec.xyz), 0 );

				float s = pow( max(dot(n.xyz,hVec.xyz), 0.f), 100.f );
				s *= coeff[ii];
				ans += s*lColor[ii]*sCoeff;
			}
		}
	}
	return ans;
}


[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void PostProcessKernel( uint3 gIdx : SV_DispatchThreadID,
						uint3 wgIdx : SV_GroupID,
						uint3 lIdx : SV_GroupThreadID,
						uint lIdx1D : SV_GroupIndex )
{
	int tileIdx=0;

	int ti = tileIdx%NUM_TILES_PER_CLUSTER;
	int tj = tileIdx/NUM_TILES_PER_CLUSTER;

	uint2 tIdx = uint2(wgIdx.x*CLUSTER_SIZE + ti*TILE_SIZE, wgIdx.y*CLUSTER_SIZE + tj*TILE_SIZE);
	uint xIdx = lIdx.x + tIdx.x;
	uint yIdx = lIdx.y + tIdx.y;

	Aabb2D tileAabb;
	tileAabb.m_min = float2( (float)tIdx.x*DX-1.f, 1.f-(float)tIdx.y*DY );
	tileAabb.m_max = float2( (float)(tIdx.x+TILE_SIZE-1)*DX-1.f, 
		1.f-(float)(tIdx.y+TILE_SIZE-1)*DY );

	float4 c = colorIn.Load( uint3(xIdx,yIdx,0) );
	float4 p = posIn.Load( uint3(xIdx,yIdx,0) );
	float4 n = normalIn.Load( uint3(xIdx,yIdx,0) );

	float4 ans = float4(0,0,0,0);
	{
		float s = 0.15f;
		ans = float4(s,s,s,0);
	}

	if( lIdx1D == 0 )
	{
		ldsLightClusterCounter = 0;
	}

	GroupMemoryBarrierWithGroupSync();

	if( lIdx1D < MAX_LIGHTS/MAX_LIGHTS_PER_TILE )
	{
		float4 lwp = lightCPos[lIdx1D];
		float fallOff = lwp.w;
		lwp.w = 1.f;
		lwp = mul( lwp, View );
		lwp.w = fallOff;
		Aabb2D lightAabb = calcAabb2D( lwp );

		if( Aabb2DOverlaps( tileAabb, lightAabb ) )
		{
			uint dst;
			InterlockedAdd( ldsLightClusterCounter, 1, dst );

			ldsLightCIdx[dst] = lIdx1D;
		}
	}

	GroupMemoryBarrierWithGroupSync();

	if( lIdx1D == 0 )
	{
		ldsLightClusterCounter = min( ldsLightClusterCounter, MAX_LIGHTS/MAX_LIGHTS_PER_TILE );
	}

	GroupMemoryBarrierWithGroupSync();

	for(int il=0; il<ldsLightClusterCounter; il++)
	{
		if( lIdx1D == 0 )
		{
			ldsLightCounter = 0;
		}

		GroupMemoryBarrierWithGroupSync();

		for(int i=lIdx1D; i<MAX_LIGHTS_PER_TILE;i+=TILE_SIZE*TILE_SIZE)
		{
			int lightIdx = ldsLightCIdx[il]*MAX_LIGHTS_PER_TILE + i;
			float4 lwp = lightPos[lightIdx];
			float fallOff = lwp.w;
			lwp.w = 1.f;
			lwp = mul( lwp, View );
			lwp.w = fallOff;
			Aabb2D lightAabb = calcAabb2D( lwp );

			if( Aabb2DOverlaps( tileAabb, lightAabb ) )
			{
				uint dst;
				InterlockedAdd( ldsLightCounter, 1, dst );

				ldsLightPos[dst] = lwp;
				ldsLightCol[dst] = lightCol[lightIdx];
			}
		}

		GroupMemoryBarrierWithGroupSync();

		{
			float4 vVec = float4(-normalize(p.xyz),0);
			for(int i=0; i<ldsLightCounter; i++)
			{
				ans += calcLighting( vVec, p, n, ldsLightPos[i], ldsLightCol[i] );
			}
		}
	}
	bufferOut[xIdx + yIdx*G_WIDTH] = ans*c;

}

