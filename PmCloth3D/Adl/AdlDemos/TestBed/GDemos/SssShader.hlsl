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

struct PS_OUT
{
	float4 m_color : SV_Target0;
	float4 m_pos : SV_Target1;
	float4 m_normal : SV_Target2;
};


float specular(float4 p, float4 l, float4 n, float4 e, float coeff)
{
	float4 h = float4( normalize( normalize(l.xyz-p.xyz) + normalize(e.xyz-p.xyz) ), 0.f );
	return pow( max( dot(n.xyz, h.xyz), 0.f), coeff );
}

float intersectSphere( float4 c, float r, float4 rayOrig, float4 rayDir )
{
	float4 op = rayOrig - c; op.w = 0.f;
	float4 d = rayDir; d.w = 0.f;
	
	float b = dot( op, d );
	float t = b*b - dot( op, op ) + r*r;

	if( t < 0.f ) return -1.f;
	t = sqrt( t );

	float dist = -b-t;
	if( dist < 0.f )
	{
		dist = -b+t;
	}
	if( dist < 0.f ) return -1.f;

	return dist;
}

#define FLT_EPSILON 1.192092896e-05F

StructuredBuffer<float4> lightBuffer : register(t0);

PS_OUT SphereSssPixelShader( PS_INPUT input)
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
	}

//	float4 lightPos = mul( float4( 0, 0, 4.f, 1.f ), View );
	float4 lightPos = mul( lightBuffer[0], View );
	lightPos /= lightPos.w;
	float4 lightDir = lightPos - posVs;
	lightDir.xyz = normalize( lightDir.xyz );

	float4 opColor = float4(0,0,0,0);
	{
		float4 ambient = float4(0.1f,0.1f,0.1f,0.f);
		float4 lightColor = float4( 0.9f, 0.9f, 0.9f, 0.f );
//		float4 lightDir = lightPos - posVs;//float4(1,1,-1,0);
		float m = 0.5f;
		opColor += saturate( dot( lightDir.xyz, normalVs.xyz ) ) * float4(m,m,m,1);// * input.Color;
//		opColor += saturate( dot( normalize(lightDir), float3(n.xy, -n.z) ) ) * input.Color;
//		opColor += ambient;
		if(0)
			opColor += specular( posVs, lightPos, output.m_normal, float4(0,0,0,0), 100.f )*lightColor;
	}

	lightPos = mul( lightBuffer[1], View );
	lightPos /= lightPos.w;
	lightDir = lightPos - posVs;
	lightDir = normalize( lightDir );

	float4 trColor;
	{
		float4 translucent = input.Color;//float4( 0.5f, 0.5f, 1.f, 1.f );
		float4 rayOrig = posVs + lightDir*FLT_EPSILON;
		float value = intersectSphere( centerVs, radius, rayOrig, lightDir );
		value = max( 0, value/(2*radius) );

//		value = ( value/2.f );
		value = pow( value, 5.f );
//		value = value*value*value;

		trColor = value*translucent;
//		trColor = lerp( translucent, float4(0,0,0,0), value );
	}

//	output.m_color = lerp( trColor, opColor, 0.2f );
//	output.m_color = trColor + float4(0.1f,0.1f,0.1f,0.f);;
	output.m_color = opColor + trColor*0.8f;
	return output;
}



PS_OUT BoxSssPixelShader( PS_INPUT input)
{
	PS_OUT output = (PS_OUT)0;
	output.m_color = input.Color;
	float alpha = output.m_color.w;

	if( input.Norm.w == 1 )
		return output;
		
	float4 ambient = float4(0.1f,0.1f,0.1f,0.f)*0.1f;
	float4 n = input.Norm; n.w = 0.f;
	n = normalize( n );
	float4 viewVec = -input.PosSS; viewVec.w = 0.f;
	viewVec = normalize( viewVec );

//	float4 lightPos = mul( lightBuffer[0], View );
//	lightPos /= lightPos.w;
//	float4 lightDir = lightPos-input.PosSS;

	float4 color;
	for(int i=0; i<2; i++)
	{
		float4 lightPos = mul( lightBuffer[i], View );
		lightPos /= lightPos.w;
		float4 lightDir = lightPos-input.PosSS; lightDir.w = 0.f;
		lightDir = normalize(lightDir);

		if( i==0 )
			color += saturate( dot( lightDir, n ) ) * input.Color;

		//	this only looks like a specular!
		if( i!=0 )
			color += pow( saturate( dot( -lightDir, viewVec ) ), 25.f ) * float4(1,1,0,0);//input.Color;
	}
	
//	color += ambient;

	output.m_color = color;
	output.m_color.w = alpha;
	return output;
}
