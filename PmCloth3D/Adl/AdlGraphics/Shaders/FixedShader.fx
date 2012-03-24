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
    float4 NormSS : TEXCOORD3;	//	view space
    float4 PosSS : TEXCOORD4;	//	view space
    float4 Color : COLOR;
	float4 PosWs : TEXCOORD5;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------


PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    
    output.Pos = mul( input.Pos, World );
	output.PosWs = output.Pos;
    output.Pos = mul( output.Pos, View );
    output.PosSS = output.Pos/output.Pos.w;
    output.Pos = mul( output.Pos, Projection );
    output.Norm = mul( float4(input.Norm.xyz,0), World );
    output.Norm.w = 0;
    if( input.Norm.x == 0 && input.Norm.y == 0 && input.Norm.z == 0 )
		output.Norm.w = 1;
		
	float4 lightDir = float4(1,1,-1,0);
	output.Light = mul( lightDir, World );
	output.Light.w = 0.f;
 
    output.Color = input.Color;
	output.Norm = normalize(output.Norm);
    output.TexCrd = input.TexCrd;
    output.NormSS = mul( float4(output.Norm.xyz,0), View );
    return output;
}

struct PS_OUT
{
	float4 m_color : SV_Target0;
};

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUT PS( PS_INPUT input)
{
	PS_OUT output = (PS_OUT)0;
	output.m_color = input.Color;
	float alpha = output.m_color.w;

	if( input.Norm.w == 1 )
		return output;
		
	float4 ambient = float4(0.1f,0.1f,0.1f,0.f)*0.1f;

	float4 lightDir = float4(1,1,-1,0)-input.PosSS;
	float4 n = input.Norm;//float4( input.Norm.xyz, 0.f );

	float4 color;
	color = saturate( dot( normalize(lightDir), n )+0.3f ) * input.Color;
	
//	color += ambient;

	output.m_color = color;
	output.m_color.w = alpha;
	return output;
}


PS_INPUT QuadVS( VS_INPUT input )
{
	PS_INPUT output = (PS_INPUT)0;
	output.Pos = input.Pos;
	output.TexCrd = input.TexCrd;
	output.Color = input.Color;
	return output;
}

PS_OUT QuadPS( PS_INPUT input)
{
	PS_OUT output = (PS_OUT)0;
	output.m_color = input.Color;
	return output;
}


StructuredBuffer<float4> buffer : register( t0 );
Texture2D texIn : register( t0 ); 


float4 BufferToRTPS( PS_INPUT Input ) : SV_TARGET
{
	int width = (int)m_gData.x;
	float4 ans = float4(1,0,0,1);
	ans = buffer[ (Input.Pos.x - 0.5) + (Input.Pos.y - 0.5) * width ];
//	ans = texIn.Load( uint3(Input.Pos.x-0.5f,Input.Pos.y-0.5f,0) );
	return ans;
}

Texture2D baseTex : register( t0 );
SamplerState defalutSam2D : register( s0 );

PS_OUT TextureMapWColorPS( PS_INPUT input)
{
	PS_OUT output = (PS_OUT)0;
	output.m_color = baseTex.Sample( defalutSam2D, input.TexCrd.xy ) * input.Color;
//	output.m_color = baseTex.Load( uint3(input.TexCrd.xy,0) ) * input.Color;
//output.m_color = float4(1,1,1,1);
	return output;
}
