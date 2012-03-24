#include <Demos/ShallowWaterEqn1DDemo.h>


ShallowWaterEqn1DDemo::ShallowWaterEqn1DDemo()
	:Demo()
{
	m_dx = 0.1f;//2.f/(float)RESOLUTION;
	m_g = -9.8f;

	m_v = new float4[RESOLUTION];
	m_wHeight = new float[RESOLUTION];
	m_gHeight = new float[RESOLUTION];

}

ShallowWaterEqn1DDemo::~ShallowWaterEqn1DDemo()
{
	delete [] m_v;
	delete [] m_wHeight;
	delete [] m_gHeight;
}

void ShallowWaterEqn1DDemo::reset()
{
	float c = RESOLUTION/2.f;
	float r = RESOLUTION/8.f/2.f;
	for(int i=0; i<RESOLUTION; i++)
	{
		m_wHeight[i] = 0.5f;

		float dist = fabs(i-c);
		if( dist < r ) m_wHeight[i] += 0.5f;//*(1.f-dist/r);

//		m_gHeight[i] = -0.5f;
//		m_gHeight[i] = -0.25f+i/(float)RESOLUTION*1.0f;
		m_gHeight[i] = max2( -0.5f, -4.f+i/(float)RESOLUTION*6.0f );
//		m_gHeight[i] = (i<RESOLUTION/2)? -0.5f: 0.f;
		m_wHeight[i] = max2( 0.f, m_wHeight[i]-m_gHeight[i] );

		m_v[i] = make_float4( 0.f );
	}
	m_v[1].x = 0.01f*m_dx/(1.f/60.f);
}

float4 ShallowWaterEqn1DDemo::interpolate( float4* v, int i, float di )
{
	i = min2( i, RESOLUTION-2 );
	i = max2( i, 0 );

	return v[i]*(1.f-di) + v[i+1]*di;
}

void ShallowWaterEqn1DDemo::step(float dt)
{
//	dt *= 0.1f;
	float c = 0.5f;
//	if(0)
	{	//	calc max velocity
		float maxV2 = 0.f;
		for(int i=0; i<RESOLUTION; i++)
		{
			float v2 = m_v[i].x*m_v[i].x;
			maxV2 = max2( maxV2, v2 );
		}

		float maxV = sqrtf(maxV2);
		if( maxV > FLT_EPSILON )
			dt = min( dt, c*m_dx/maxV );
	}

	{	//	1. advect velocity
		float4* newV = new float4[RESOLUTION];
		memcpy( newV, m_v, sizeof(float4)*(RESOLUTION-1) );
		for(int i=1; i<RESOLUTION-1; i++)
		{
			if( cellIsDry(i+1) && cellIsDry(i) ) continue;

			float pos = getPosition(i);
			float v = m_v[i].x;

			float pPos = pos - v*dt;

			float gCrd = (pPos)/m_dx;
			newV[i] = interpolate( m_v, (int)gCrd, gCrd - (int)gCrd );
		}
		memcpy( m_v, newV, sizeof(float4)*(RESOLUTION-1) );
		delete [] newV;
	}

	{	//	2. integrate height
		float* newH = new float[RESOLUTION];
		{	//	reflecting boundary
			newH[0] = m_wHeight[1];
			newH[RESOLUTION-1] = m_wHeight[RESOLUTION-2];
		}
		for(int i=1; i<RESOLUTION-1; i++)
		{
			float vi = m_v[i].x;
			int up = i;
			int dn = i-1;

			float dhvdx = ((m_wHeight[up]+m_wHeight[up+1])/2.f*m_v[up].x
				-(m_wHeight[dn]+m_wHeight[dn+1])/2.f*m_v[dn].x)/(m_dx);

			newH[i] = m_wHeight[i] - dhvdx * m_dt;

			newH[i] = max2( 0.f, newH[i] );
		}
		memcpy( m_wHeight, newH, sizeof(float)*RESOLUTION );
		delete [] newH;
	}

	{	//	3. integrate velocity
		for(int i=1; i<RESOLUTION-2; i++)
		{
			float4& v = m_v[i];

			if( cellIsDry(i+1) && cellIsDry(i) ) { v.x = 0.f; continue; }

			float dh = getSurface( i+1 ) - getSurface( i );

			v.x += m_g/m_dx*dh*dt;
		}
	}
}

void ShallowWaterEqn1DDemo::render()
{
	float e = m_dx*RESOLUTION;
	float he = e*0.5f;
	e *= 0.25f;

	if( RESOLUTION <= 128 )
	for(int i=0; i<RESOLUTION-1; i++)
	{
		float4 x = make_float4( (getPosition(i)-he)/e, -1.f, 0.f );
		float4 xx = make_float4( (getPosition(i)-he)/e, 1.f, 0.f );
		pxDrawLine( x, xx, make_float4(0.1f) );
	}

	float vScale = 0.1f;
	for(int i=0; i<RESOLUTION-2; i++)
	{
		float4 x = make_float4( (getPosition(i)-he)/e, getSurface(i), 0.f );
		float4 xx = make_float4( (getPosition(i+1)-he)/e, getSurface(i+1), 0.f );
		if( m_wHeight[i] != 0.f || m_wHeight[i+1] != 0.f )
			pxDrawLine( x, xx, make_float4( 0.f, 0.f, 1.f ) );

		x = make_float4( (getPosition(i)-he)/e, m_gHeight[i], 0.f );
		xx = make_float4( (getPosition(i+1)-he)/e, m_gHeight[i+1], 0.f );
		pxDrawLine( x, xx, make_float4( 1.f ) );


		float4 v = m_v[i]*vScale;;
		x.y += 0.01f;
		pxDrawLine( x, x+v, make_float4(0.f, 1.f, 0.f) );
	}

}
