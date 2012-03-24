#define u32 uint

#define DEFAULT_ARGS uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID
#define GET_GROUP_IDX groupIdx.x
#define GET_LOCAL_IDX localIdx.x
#define GET_GLOBAL_IDX globalIdx.x
#define GET_GLOBAL_IDY globalIdx.y
#define GROUP_LDS_BARRIER GroupMemoryBarrierWithGroupSync()


cbuffer CB0 : register( b0 )
{
	matrix m_viewInv;
	matrix m_projInv;

	matrix m_lightView;
	matrix m_lightProj;

	int m_width;
	int m_height;
	float m_shadowWeight;
	int m_shadowIdx;
};


float4 projToView(float4 p)
{
	p = mul( p, m_projInv );
	p /= p.w;
	return p;
}


#define SHADOW_EPSILON 0.00005f
#define EPSILON 0.001f

RWStructuredBuffer<float4> bufferOut : register(u0);
Texture2D normalBuffer : register( t0 );

Texture2D depthBuffer : register( t0 );
Texture2D shadowBuffer : register( t1 );


float checkShadow(float4 posWs, matrix lightViewProj)
{
	float4 posLs = mul( posWs, lightViewProj );
//	posLs = mul( posLs, lightProj );
	posLs/=posLs.w;

	float2 shadowCrd = (posLs.xy+1)/2.f;
	shadowCrd.y = 1.f-shadowCrd.y;

	if( shadowCrd.x <= EPSILON || shadowCrd.x >= 1.f-EPSILON
		|| shadowCrd.y <= EPSILON || shadowCrd.y >= 1.f-EPSILON ) return 1.f;

	uint3 shadowCrdUint = uint3( shadowCrd.x*m_width, shadowCrd.y*m_height, 0 );

	float ans = 0.f;;
	ans += ( shadowBuffer.Load( shadowCrdUint )+SHADOW_EPSILON 
		< posLs.z )? 0.0f:1.0f;
	ans += ( shadowBuffer.Load( shadowCrdUint+uint3(1,0,0) )+SHADOW_EPSILON 
		< posLs.z )? 0.0f:1.0f;
	ans += ( shadowBuffer.Load( shadowCrdUint+uint3(1,1,0) )+SHADOW_EPSILON 
		< posLs.z )? 0.0f:1.0f;
	ans += ( shadowBuffer.Load( shadowCrdUint+uint3(0,1,0) )+SHADOW_EPSILON 
		< posLs.z )? 0.0f:1.0f;

	return ans/4.f;
}

[numthreads(64, 1, 1)]
void ClearKernel( DEFAULT_ARGS )
{
	int gIdx = GET_GLOBAL_IDX;
	
	uint xIdx = gIdx%m_width;
	uint yIdx = uint(gIdx/m_width);

	float4 n = normalBuffer.Load( uint3(xIdx,yIdx,0) );

	bufferOut[gIdx] = max( 0.1f, dot( n.xyz, normalize( float3(8,8,-8) ) ) );
}

[numthreads(8, 8, 1)]
void ShadowAccmKernel( DEFAULT_ARGS )
{
	uint xIdx = GET_GLOBAL_IDX;
	uint yIdx = GET_GLOBAL_IDY;

	uint gIdx = xIdx+yIdx*m_width;

	float depth = depthBuffer.Load( uint3(xIdx,yIdx,0) );

	if( depth == 1.f )
	{
		return;
	}

	float4 posW;
	{	//	restore view position
		float4 projCrd = float4(xIdx/(float)m_width*2.f-1.f, (m_height-yIdx)/(float)m_height*2.f-1.f, depth, 1.f);
//		float4 posV = projToView( projCrd );
//		posW = mul( posV, m_viewInv );
		posW = mul( projCrd, m_viewInv );
		posW /= posW.w;
	}

	float shadow = checkShadow( posW, m_lightView );

	bufferOut[gIdx] -= (1.f-shadow)*m_shadowWeight;
}


RWStructuredBuffer<float> lightMergedBufferOut : register(u0);

[numthreads(64, 1, 1)]
void CopyShadowMapKernel( DEFAULT_ARGS )
{
	int gIdx = GET_GLOBAL_IDX;
	
	uint xIdx = gIdx%m_width;
	uint yIdx = uint(gIdx/m_width);

	float d = depthBuffer.Load( uint3(xIdx,yIdx,0) );

	lightMergedBufferOut[gIdx+m_shadowIdx*m_width*m_height] = d;
}


StructuredBuffer<float> lightMergedBuffer : register(t1);
StructuredBuffer<matrix> lightMatrixBuffer : register(t2);

float checkShadow(int offset, float4 posWs, matrix lightViewProj)
{
	float4 posLs = mul( posWs, lightViewProj );
	posLs/=posLs.w;

	float2 shadowCrd = (posLs.xy+1)/2.f;
	shadowCrd.y = 1.f-shadowCrd.y;

	if( shadowCrd.x <= EPSILON || shadowCrd.x >= 1.f-EPSILON
		|| shadowCrd.y <= EPSILON || shadowCrd.y >= 1.f-EPSILON ) return 1.f;

	uint3 shadowCrdUint = uint3( shadowCrd.x*m_width, shadowCrd.y*m_height, 0 );

	float ans = 0.f;;
	ans += ( lightMergedBuffer[offset+(shadowCrdUint.x)+(shadowCrdUint.y)*m_width]+SHADOW_EPSILON 
		< posLs.z )? 0.0f:1.0f;
	ans += ( lightMergedBuffer[offset+(shadowCrdUint.x+1)+(shadowCrdUint.y)*m_width]+SHADOW_EPSILON 
		< posLs.z )? 0.0f:1.0f;
	ans += ( lightMergedBuffer[offset+(shadowCrdUint.x+1)+(shadowCrdUint.y+1)*m_width]+SHADOW_EPSILON 
		< posLs.z )? 0.0f:1.0f;
	ans += ( lightMergedBuffer[offset+(shadowCrdUint.x)+(shadowCrdUint.y+1)*m_width]+SHADOW_EPSILON 
		< posLs.z )? 0.0f:1.0f;

	return ans/4.f;
}

[numthreads(8, 8, 1)]
void ShadowAccmAllKernel( DEFAULT_ARGS )
{
	uint xIdx = GET_GLOBAL_IDX;
	uint yIdx = GET_GLOBAL_IDY;

	uint gIdx = xIdx+yIdx*m_width;

	float depth = depthBuffer.Load( uint3(xIdx,yIdx,0) );

	if( depth == 1.f )
	{
		return;
	}

	float4 posW;
	{	//	restore view position
		float4 projCrd = float4(xIdx/(float)m_width*2.f-1.f, (m_height-yIdx)/(float)m_height*2.f-1.f, depth, 1.f);
		posW = mul( projCrd, m_viewInv );
		posW /= posW.w;
	}

	float sum = 0.f;
	for(int i=0; i<m_shadowIdx; i++)
	{
		float shadow = checkShadow( i*m_width*m_height, posW, lightMatrixBuffer[i] );
		sum += 1.f-shadow;
	}

	bufferOut[gIdx] -= sum*m_shadowWeight;
}


/*
Texture2D shadowBuffer0 : register( t1 );
Texture2D shadowBuffer1 : register( t2 );
Texture2D shadowBuffer2 : register( t3 );
Texture2D shadowBuffer3 : register( t4 );
Texture2D shadowBuffer4 : register( t5 );

Texture2D shadowBuffer5 : register( t6 );
Texture2D shadowBuffer6 : register( t7 );
Texture2D shadowBuffer7 : register( t8 );
Texture2D shadowBuffer8 : register( t9 );
Texture2D shadowBuffer9 : register( t10 );


cbuffer CB0 : register( b0 )
{
	matrix m_viewInv;

	matrix m_lightMat0;
	matrix m_lightMat1;
	matrix m_lightMat2;
	matrix m_lightMat3;
	matrix m_lightMat4;

	matrix m_lightMat5;
	matrix m_lightMat6;
	matrix m_lightMat7;
	matrix m_lightMat8;
	matrix m_lightMat9;

	int m_width;
	int m_height;
	float m_shadowWeight;
};


[numthreads(8, 8, 1)]
void ShadowAccm10Kernel( DEFAULT_ARGS )
{
	uint xIdx = GET_GLOBAL_IDX;
	uint yIdx = GET_GLOBAL_IDY;

	uint gIdx = xIdx+yIdx*m_width;

	float depth = depthBuffer.Load( uint3(xIdx,yIdx,0) );

	if( depth == 1.f )
	{
		return;
	}

	float4 posW;
	{	//	restore view position
		float4 projCrd = float4(xIdx/(float)m_width*2.f-1.f, (m_height-yIdx)/(float)m_height*2.f-1.f, depth, 1.f);
		posW = mul( projCrd, m_viewInv );
		posW /= posW.w;
	}

	float shadow = checkShadow( posW, m_lightView, m_lightProj );

	bufferOut[gIdx] -= (1.f-shadow)*m_shadowWeight;
}
*/



