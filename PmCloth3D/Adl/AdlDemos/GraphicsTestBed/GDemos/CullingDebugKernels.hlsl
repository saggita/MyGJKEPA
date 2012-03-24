#define u32 uint

#define DEFAULT_ARGS uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID
#define GET_GROUP_IDX groupIdx.x
#define GET_LOCAL_IDX localIdx.x
#define GET_GLOBAL_IDX globalIdx.x
#define GROUP_LDS_BARRIER GroupMemoryBarrierWithGroupSync()


RWStructuredBuffer<float4> bufferOut : register(u0);

StructuredBuffer<u32> lowerBound : register(t0);
StructuredBuffer<u32> upperBound : register(t1);
StructuredBuffer<int2> lightIndexBuffer :register(t2);
StructuredBuffer<float4> lightBuffer : register(t3);
Texture2D colorIn : register( t4 );
Texture2D depthIn : register( t5 );
StructuredBuffer<u32> lightBitsBuffer : register(t6);

#define MAX_LIGHTS_PER_TILE 64

cbuffer CB0 : register( b0 )
{
	matrix m_view;
	matrix m_projection;
	matrix m_projectionInv;
	int m_width;
	int m_height;
	int m_tileRes;
	int m_padding;
};

int getNCellsX()
{
	return (m_width+m_tileRes-1)/m_tileRes;
}

float4 projToView(float4 p)
{
	p = mul( p, m_projectionInv );
	p /= p.w;
	return p;
}

groupshared float4 ldsLights[MAX_LIGHTS_PER_TILE];

[numthreads(64, 1, 1)]
void DisplayKernel( DEFAULT_ARGS )
{
	const float cellCap = 25.f;

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

		float4 ans = float4(0.f,0.f,0.f,0.f);

		float4 cb = colorIn.Load( uint3(xIdx,yIdx,0) );
		float depth = depthIn.Load( uint3(xIdx,yIdx,0) );

		{	//	set from color buffer
			ans = cb * 0.5f;
//			ans *= 1.f-float4(n, n, n, n)/cellCap;
		}

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
						ans += float4(0.0f, 0.2f, 0.0f, 0.f);
					}
				}
			}

			if( n > MAX_LIGHTS_PER_TILE ) ans = 1.f-ans;
		}

		if( xIdx%m_tileRes == 0 || yIdx%m_tileRes == 0 ) ans = float4(0.f, 0.f, 0.f, 0.f);

		bufferOut[pixelIdx] = ans;
	}
}


[numthreads(64, 1, 1)]
void DisplayPixelBitsKernel( DEFAULT_ARGS )
{
	const float cellCap = 5.f;

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
		uint lightBits = lightBitsBuffer[pixelIdx];

		float4 ans = float4(0.f,0.f,0.f,0.f);

		float4 cb = colorIn.Load( uint3(xIdx,yIdx,0) );
		float depth = depthIn.Load( uint3(xIdx,yIdx,0) );

		{	//	set from color buffer
			ans = cb * 0.9f;
			n = 0;
			for(int i=0; i<32; i++)
			{
				if( (lightBits>>i)&1 )
				{
					//	overlapping to ldsLights[i]
					n++;
				}
			}
			ans *= 1.f-float4(n, n, 0, 0)/cellCap;
		}

		if( xIdx%m_tileRes == 0 || yIdx%m_tileRes == 0 ) ans = float4(0.f, 0.f, 0.f, 0.f);

		bufferOut[pixelIdx] = ans;
	}
}



