cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 m_gData;
}


struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Norm : TEXCOORD0;
    float4 Light : TEXCOORD1;
    float4 TexCrd : TEXCOORD2;
    float4 NormSS : TEXCOORD3;
    float4 PosSS : TEXCOORD4;
    float4 Color : COLOR;
};

struct PS_OUTPUT
{
	float4 m_color0 : SV_Target0;
//	float4 m_pos : SV_Target1;
//	float4 m_normal : SV_Target2;
};

PS_OUTPUT DepthPS( PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;
	output.m_color0 = input.Color;

	output.m_color0.w = input.PosSS.z;
	output.m_color0.xyz = input.Color.xyz;

	return output;
}

