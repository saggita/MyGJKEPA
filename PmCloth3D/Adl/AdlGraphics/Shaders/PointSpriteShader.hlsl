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

static const float2 g_positions[4] = { float2(-1, 1), float2(1, 1), float2(-1, -1), float2(1, -1) };
static const float2 g_texcoords[4] = { float2(0, 1), float2(1, 1), float2(0, 0), float2(1, 0) };

[maxvertexcount(8)]
void ParticleGS(point PS_INPUT In[1], 
	inout TriangleStream<PS_INPUT> SpriteStream)
{
	//	radius
	float pSize = In[0].TexCrd.z;//m_gData.x;
    for (int i = 0; i < 4; i++)
    {
        PS_INPUT Out = In[0];
		float4 p = In[0].PosSS.xyzw;
		p /= p.wwww;
        float4 pp = p + pSize * float4(g_positions[i], 0, 0);
		Out.PosSS = pp;
		Out.PosSS.w = pSize;
		Out.NormSS = p;
		Out.Pos = mul( pp, Projection );
        Out.TexCrd = float4(g_texcoords[i].xy, 0,0);		
        SpriteStream.Append(Out);
    }
    SpriteStream.RestartStrip();
}

struct PS_OUT
{
	float4 m_color : SV_Target0;
	float4 m_pos : SV_Target1;
	float4 m_normal : SV_Target2;
	float m_depth : SV_Depth;
};

float specular(float4 p, float4 l, float4 n, float4 e, float coeff)
{
	float4 h = float4( normalize( normalize(l.xyz-p.xyz) + normalize(e.xyz-p.xyz) ), 0.f );
	return pow( max( dot(n.xyz, h.xyz), 0.f), coeff );
}

PS_OUT PS( PS_INPUT input)
{
	float alpha = input.Color.w;
	PS_OUT output = (PS_OUT)0;
	float rad = 0.5f;
	float2 crd = input.TexCrd.xy - float2(rad, rad);
	if( length(crd) > rad ) discard;

	float z2 = 0.5f*0.5f - dot(crd, crd);
	float3 n = float3(crd.xy, sqrt(z2));

	n = normalize( n );

	float4 posVs;
	float4 normalVs;
	float4 centerVs;
	float radius;
	{
		centerVs = input.NormSS;
		float4 posXY = input.PosSS;
		float4 dxy = posXY-centerVs;
		dxy.zw = 0.f;
		float r = posXY.w;
		float z = sqrt( r*r - dot( dxy, dxy ) );
		radius = r;

		posVs = float4( posXY.xy, posXY.z - z, 0.f );
		normalVs = posVs - centerVs; normalVs.w = 0.f;
		normalVs = normalize( normalVs );

		output.m_pos = posVs;
		output.m_normal = normalVs;

		posVs.w = 1.f;
		float4 posPs = mul( posVs, Projection );
		output.m_depth = posPs.z/posPs.w;
	}


	float4 ambient = float4(0.1f,0.1f,0.1f,0.f);
	float4 lightPos = float4( 10, 10, -10, 0 );
	float4 lightColor = input.Color+float4(0.5f,0,0,0);//float4( 0.9f, 0.9f, 0.f, 0.f );
	float4 lightDir = lightPos - posVs;//float4(1,1,-1,0);
	output.m_color = saturate( dot( normalize(lightDir), float3(n.xy, -n.z) ) ) * input.Color;
	output.m_color += ambient;
	if(1)
		output.m_color += specular( posVs, lightPos, output.m_normal, float4(0,0,0,0), 100.f )*lightColor;

	if(1)
	{	//	lim shadow
		float4 up = float4(0,0,1,0);
		output.m_color *= pow( max( dot( up.xyz, normalize(n.xyz) ), 0.f), 1.f );
	}

	output.m_color.w = alpha;

	return output;
/*
	float alpha = input.Color.w;
	PS_OUT output = (PS_OUT)0;
	float rad = 0.5f;
	float2 crd = input.TexCrd.xy - float2(rad, rad);
	if( length(crd) > rad ) discard;

	float z2 = 0.5f*0.5f - dot(crd, crd);
	float3 n = float3(crd.xy, sqrt(z2));

	n = normalize( n );

	float4 posSS;
	{
		float4 center = input.NormSS;
		float4 posXY = input.PosSS;
		float4 dxy = posXY-center;
		dxy.zw = 0.f;
		float r = posXY.w;
		float z = sqrt( r*r - dot( dxy, dxy ) );

		posSS = float4( posXY.xy, posXY.z - z, 0.f );

		output.m_pos = posSS;

		output.m_normal = normalize( dxy-float4(0,0,z,0) );
		output.m_depth = posSS.z;
	}

	float4 ambient = float4(0.1f,0.1f,0.1f,0.f);
	float4 lightPos = float4( 10, 10, -10, 0 );
	float4 lightColor = input.Color+float4(0.5f,0,0,0);//float4( 0.9f, 0.9f, 0.f, 0.f );
	float4 lightDir = lightPos - posSS;//float4(1,1,-1,0);
	output.m_color = saturate( dot( normalize(lightDir), float3(n.xy, -n.z) ) ) * input.Color;
	output.m_color += ambient;
	if(1)
		output.m_color += specular( posSS, lightPos, output.m_normal, float4(0,0,0,0), 100.f )*lightColor;

	if(1)
	{	//	lim shadow
		float4 up = float4(0,0,1,0);
		output.m_color *= pow( max( dot( up.xyz, normalize(n.xyz) ), 0.f), 1.f );
	}

	output.m_color.w = alpha;

	return output;
*/
}

/*
PS_OUT PS( PS_INPUT input)
{
	PS_OUT output = (PS_OUT)0;
	float rad = 0.5f;
	float2 crd = input.TexCrd.xy - float2(rad, rad);
	if( length(crd) > rad ) discard;
	
	output.m_color = float4(input.TexCrd.xy,0,1);

	return output;
}
*/


struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Norm : NORMAL;
    float4 Color : COLOR;
    float4 TexCrd : TEXCOORD0;
    uint m_vtxIdx : SV_InstanceID;
};

StructuredBuffer<float4> posTex : register( t0 );
StructuredBuffer<float4> colorTex : register( t1 );

PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    
    float4 p = posTex[ input.m_vtxIdx ];
    p.w = 1.f; 
    
    output.Pos = mul( p, World );
    output.Pos = mul( output.Pos, View );
    output.PosSS = output.Pos;
    output.Pos = mul( output.Pos, Projection );
    output.Norm = float4(0,0,0,0);
		
	float4 lightDir = float4(1,1,-1,0);
	output.Light = mul( lightDir, World );
	output.Light.w = 0.f;
 
    output.Color = colorTex[ input.m_vtxIdx ];//float4(1,1,1,1);//input.Color;
    output.NormSS = mul( float4(output.Norm.xyz,0), View );
    return output;
}


/*
[maxvertexcount(8)]
void FontGS(point PS_INPUT In[1], 
	inout TriangleStream<PS_INPUT> SpriteStream)
{
	//	radius
	float pSize = In[0].TexCrd.x;//m_gData.x;
    for (int i = 0; i < 4; i++)
    {
        PS_INPUT Out = In[0];
		float4 p = In[0].PosSS.xyzw;
		p /= p.wwww;
        float4 pp = p + pSize * float4(g_positions[i], 0, 0);
		Out.PosSS = pp;
		Out.PosSS.w = pSize;
		Out.NormSS = p;
		Out.Pos = mul( pp, Projection );
        Out.TexCrd = float4(g_texcoords[i].xy, 0,0);		
        SpriteStream.Append(Out);
    }
    SpriteStream.RestartStrip();
}
*/