//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 m_gData;
}

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Norm : NORMAL;
    float4 Color : COLOR;
    float4 TexCrd : TEXCOORD0;
    uint m_vtxIdx : SV_VertexID;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Norm : TEXCOORD0;
    float4 Light : TEXCOORD1;
    float4 TexCrd : TEXCOORD2;
    float4 m_normWS : TEXCOORD3;
    float4 m_posWS : TEXCOORD4;
    float4 Color : COLOR;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT DeferredGPVS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.m_posWS = output.Pos;
	output.m_posWS /= output.m_posWS.w;

    output.Pos = mul( output.Pos, Projection );
    output.m_normWS = mul( float4(input.Norm.xyz,0), World );
	output.m_normWS = mul( float4(input.Norm.xyz,0), View );

	output.Color = input.Color;


    return output;
}


struct PS_OUTPUT
{
	float4 m_color : SV_Target0;
	float4 m_pos : SV_Target1;
	float4 m_normal : SV_Target2;
};


PS_OUTPUT DeferredGPPS( PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;

	output.m_color = input.Color;
	output.m_pos = input.m_posWS;
	output.m_normal = input.m_normWS;

	return output;
}

