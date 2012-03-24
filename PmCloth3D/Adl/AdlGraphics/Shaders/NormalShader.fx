
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Norm : TEXCOORD0;
    float4 Light : TEXCOORD1;
    float4 Color : COLOR;
};


float4 PS( PS_INPUT input) : SV_Target
{
	float4 n = float4( input.Norm.xyz, 0.f );

	n = normalize( n );
	n.z *= -1;

	return n;
}

