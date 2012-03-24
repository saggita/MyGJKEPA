Texture2D colorIn : register( t0 ); 
Texture2D depthIn : register( t1 ); 
RWStructuredBuffer<float4> bufferOut : register(u0);


cbuffer ConstantBuffer : register( b0 )
{
	int m_width;
	int m_height;
}



[numthreads(64, 1, 1)]
void PostProcessKernel( uint3 gIdx : SV_DispatchThreadID )
{
	float4 ans = float4(0.f,0.f,0.f,0.f);

	const uint WIDTH = m_width;

	uint xIdx = gIdx.x%WIDTH;
	uint yIdx = uint(gIdx.x/WIDTH);

	int stride = 5;

	for(int i=-stride; i<=stride; i++)
	{
		ans += colorIn.Load( uint3(xIdx+i,yIdx,0) );
	}

    bufferOut[gIdx.x] = ans/(stride*2+1);

	if( xIdx < WIDTH/2 )
	{
		float d = ((depthIn.Load( uint3(xIdx, yIdx, 0) )-0.8f)/0.2f);
		bufferOut[gIdx.x] = d;
	}
}


