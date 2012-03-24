struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Norm : TEXCOORD0;
    float4 Light : TEXCOORD1;
    float4 TexCrd : TEXCOORD2;
    float4 NormSS : TEXCOORD3;	//	view space
    float4 PosSS : TEXCOORD4;	//	view space
    float4 Color : COLOR;
	float4 PosWs : TEXCOORD5;
};

struct PS_OUT
{
	float4 m_color : SV_Target0;
};

cbuffer ConstantBuffer : register( b1 )
{
	matrix m_lightView;
	matrix m_lightProjection;
}



Texture2D shadowBuffer : register( t0 ); 
SamplerState defalutSam2D : register( s0 );


#define SHADOW_EPSILON 0.00005f

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUT ShadowMapPS( PS_INPUT input)
{
	PS_OUT output = (PS_OUT)0;
	output.m_color = input.Color;
	float alpha = output.m_color.w;

	if( input.Norm.w == 1 )
		return output;


	float inShadow;
	{
		float4 posVs = input.PosWs;
		float4 posLs = mul( posVs, m_lightView );
		posLs = mul( posLs, m_lightProjection );
		posLs/=posLs.w;

		float2 shadowCrd = (posLs.xy+1)/2.f;
		shadowCrd.y = 1.f-shadowCrd.y;

		inShadow = ( shadowBuffer.Sample( defalutSam2D, shadowCrd )+SHADOW_EPSILON < posLs.z )? 0.5f:1.0f;
	}
		
	float4 ambient = float4(0.1f,0.1f,0.1f,0.f)*0.1f;

	float4 lightDir = float4(1,1,-1,0)-input.PosSS;
	float4 n = input.Norm;

	float4 color;
	color = saturate( dot( normalize(lightDir), n )+0.3f ) * input.Color;
	
	output.m_color = color*inShadow;
	output.m_color.w = alpha;
	return output;
}


