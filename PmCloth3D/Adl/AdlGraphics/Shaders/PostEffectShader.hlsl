cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
	float4 m_gData;
}


Texture2D baseTex : register( t0 );
Texture2D posTex : register( t1 );
Texture2D normalTex : register( t2 );

SamplerState defalutSam2D : register( s0 );


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
	float4 m_pos : SV_Target1;
	float4 m_normal : SV_Target2;
};


float4 PS( PS_INPUT input) : SV_Target
{
	return baseTex.Sample( defalutSam2D, input.TexCrd.xy );
}

PS_OUTPUT MultiPS( PS_INPUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;
	output.m_color0 = input.Color;
	if( input.Norm.w == 1 )
		return output;
		
	float4 ambient = float4(0.1f,0.1f,0.1f,0.f);

	float4 lightDir = float4(1,1,-1,0);
	float4 n = float4( input.Norm.xyz, 0.f );
	n = normalize( n );

	float4 color;
	color = saturate( dot( normalize(lightDir), n ) ) * input.Color;
	
	color += ambient;

	output.m_color0 = color;

	output.m_pos = input.PosSS;
	output.m_normal = input.NormSS;

	return output;
}



struct PS_OUTPUT1
{
	float4 m_color : SV_Target0;
};

#define GET_POS(uv) posTex.Sample( defalutSam2D, uv )
#define GET_NORMAL(uv) normalTex.Sample( defalutSam2D, uv )


float calcAO(float2 uv, float4 p, float4 n)
{
	float g_scale = 1.f;
	float g_bias = 0.f;
	float g_intensity = 3.f;

	float4 jPos = GET_POS(uv);
	float4 diff = jPos - p; diff.w = 0.f;
	if( jPos.x == 0.f ) return 0.f;
	const float4 v = normalize(diff);
	const float d = length(diff)*g_scale;
	return max(0.0,dot(n,v)-g_bias)*(1.0/(1.0+d))*g_intensity;
}

//	why resize doesn't work??
PS_OUTPUT1 PostPS( PS_INPUT input)
{
	PS_OUTPUT1 output;

	const float2 vec[4] = {float2(1,0),float2(-1,0),
            float2(0,1),float2(0,-1)};

	float4 iPos = GET_POS(input.TexCrd.xy);
	float4 iNormal = GET_NORMAL(input.TexCrd.xy);

	float ao = 0.f;
	{
		const int size = 2;
		for(int i=-size; i<=size; i++)for(int j=-size; j<=size; j++)
		{
			if( i==0 && j==0 ) continue;
			float s = 3.f/800.f;
			float2 v = float2(i*s, j*s);
			ao += calcAO(input.TexCrd.xy + v, iPos, iNormal);
		}
		ao /= pow((size*2+1), 2.f) - 1;
	}

	ao = 1.f-min(0.95f, ao);

	output.m_color = float4(ao, ao, ao, 1.f);
//	output.m_color = iPos;
//	output.m_color = iNormal;

	return output;
}


Texture2D baseTex2 : register( t0 );
Texture2D aoTex2 : register( t1 );

struct PS_OUTPUT2
{
	float4 m_color : SV_Target0;
};

PS_OUTPUT2 FinalPS( PS_INPUT input )
{
	PS_OUTPUT2 output = (PS_OUTPUT2)0;

	float4 base = baseTex2.Sample( defalutSam2D, input.TexCrd.xy );
	float4 ao = aoTex2.Sample( defalutSam2D, input.TexCrd.xy );

	output.m_color = base*ao;

	return output;
}