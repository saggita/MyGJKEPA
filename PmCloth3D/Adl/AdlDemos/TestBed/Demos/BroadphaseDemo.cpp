#include <Demos/BroadphaseDemo.h>
#include <Demos/Bp/BFBroadphase.h>
#include <Demos/Bp/SweepBroadphase.h>


BroadphaseDemo::BroadphaseDemo(const Device* device)
	: Demo( device )
{
	m_pos = new float4[ MAX_PARTICLES ];
	m_vel = new float4[ MAX_PARTICLES ];
	m_force = new float4[ MAX_PARTICLES ];
	m_aabb = new Aabb[ MAX_PARTICLES ];
	m_aabbUint = new AabbUint[ MAX_PARTICLES ];

	m_planes[0] = make_float4(0,1,0,1);
	m_planes[1] = make_float4(-1,0,0,1);
	m_planes[2] = make_float4(1,0,0,1);
	m_planes[3] = make_float4(0,-1,0,1);
}

BroadphaseDemo::~BroadphaseDemo()
{
	delete [] m_pos;
	delete [] m_vel;
	delete [] m_force;
	delete [] m_aabb;
	delete [] m_aabbUint;
}

void BroadphaseDemo::reset()
{
	float s = 1.f;
	float r = 0.1f/2.f;
	float density = 100.f;

	for(int i=0; i<MAX_PARTICLES; i++)
	{
		m_pos[i] = make_float4( getRandom( -s, s ), getRandom( -s, s ), 0.f );
		float mr = m_pos[i].w = getRandom( r/2.f, r );
		m_vel[i] = make_float4( 0, 0, 0, mr*mr*PI*density );
		m_force[i] = make_float4( 0.f );
	}

}

__inline
void calcBoundary(float4* p0, float4* v0, float4* fOut0, int n0, float dt, float e,
							float4* planes, int NUM_PLANES)
{
	for(int i=0; i<n0; i++)
	{
		float4 f = make_float4(0,0,0,0);
		float sCoeff, dCoeff;
		{
			float m = v0[i].w/2.f;
			sCoeff = m/(dt*dt);
			dCoeff = m/dt*(1.f-e);
		}
		for(int j=0; j<NUM_PLANES; j++)
		{
			const float4& eqn = planes[j];
			float dist = dot3w1( p0[i], eqn );
			float r_i = p0[i].w;
			if( dist < r_i )
			{
				f += sCoeff*(r_i-dist)*eqn;
				f += dCoeff*(-v0[i]);
			}
		}
		fOut0[i] += f;
	}
}

__inline
void integration(float4* p0, float4* v0, float4* f0, int n0, float dt, const float4& g)
{
	for(int i=0; i<n0; i++)
	{

		if( v0[i].w == FLT_MAX ) continue;


		float4 x = p0[i];
		float4 v = v0[i];

		v += f0[i]*dt/v.w+g;
		x += v*dt;

		p0[i] = make_float4(x.x, x.y, x.z, p0[i].w);
		v0[i] = make_float4(v.x, v.y, v.z, v0[i].w);
		f0[i] = make_float4(0,0,0,0);
	}
}

__inline
float4 calcForce(const float4& x_i, const float4& x_j, const float4& v_i, const float4& v_j, float r_i, float r_j, float m_i, float m_j, float dt,
				 float e = 0.7f)
{
	float4 f = make_float4(0,0,0,0);
	float sCoeff, dCoeff;

	{
		float dtInv = 1.f/dt;
		float m = (m_i*m_j)/(m_i+m_j);
		sCoeff = m*dtInv*dtInv;
		dCoeff = m*dtInv*(1.f-e);
	}

	float4 x_ij = x_j-x_i;
	float dist2 = dot3F4( x_ij, x_ij );

	if( dist2 < pow( r_i+r_j, 2.f ) )
	{
		float dist = sqrtf( dist2 );
		f -= sCoeff*(r_i+r_j-dist)*x_ij/dist;
		f += dCoeff*(v_j - v_i);
	}
	return f;
}


void BroadphaseDemo::step( float dt )
{
	dt *= 0.0125f;

	float e = 0.85f;
	float4 g = make_float4(0.f, -9.8f, 0.f, 0.f) * 0.5f;

	for(int i=0; i<MAX_PARTICLES; i++)
	{
		float r = m_pos[i].w;
		const float4& c = m_pos[i];
		m_aabb[i].m_max = c + make_float4(r);
		m_aabb[i].m_min = c - make_float4(r);
	}

	{
		Aabb space;
		space.m_max = make_float4(1.5f,1.5f,0.1f);
		space.m_min = make_float4(-1.5f,-1.5f,-0.1f);
		convertToAabbUint( m_aabb, m_aabbUint, MAX_PARTICLES, space );
	}

	Pair32* pairs = new Pair32[PAIR_CAPACITY];

	Stopwatch sw;
	{
		sw.start();
		BFBroadphase bp;
		int nPairs = bp.getPair( m_aabbUint, MAX_PARTICLES, pairs, PAIR_CAPACITY );

		sw.split();
		SweepBroadphase bpsp;
		nPairs = bpsp.getPair( m_aabbUint, MAX_PARTICLES, pairs, PAIR_CAPACITY );
		sw.stop();

		for(int i=0; i<nPairs; i++)
		{
			int aIdx = pairs[i].m_a;
			int bIdx = pairs[i].m_b;

			float4 f = calcForce( m_pos[aIdx], m_pos[bIdx], m_vel[aIdx], m_vel[bIdx], m_pos[aIdx].w, m_pos[bIdx].w, m_vel[aIdx].w, m_vel[bIdx].w,
				dt, e );

			m_force[aIdx] += f;
			m_force[bIdx] -= f;
		}
	}

	delete [] pairs;

	calcBoundary( m_pos, m_vel, m_force, MAX_PARTICLES, dt, e, m_planes, 4 );

	integration( m_pos, m_vel, m_force, MAX_PARTICLES, dt, g );

	{
		float t[2];
		sw.getMs( t, 2 );
		m_nTxtLines = 0;
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "%3.2fms, %3.2fms", t[0], t[1]);
	}
}

void BroadphaseDemo::render()
{
	float inc = 0.45f;
	{
		float4* p = new float4[MAX_PARTICLES];
		float4* c = new float4[MAX_PARTICLES];
		float2* r = new float2[MAX_PARTICLES];

		for(int i=0; i<MAX_PARTICLES; i++)
		{
			p[i] = m_pos[i];
			c[i] = make_float4(0.f, 1.f, 0.f);
			r[i].x = m_pos[i].w;
		}

		pxDrawPointSprite( p, c, r, MAX_PARTICLES );

		delete [] p;
		delete [] c;
		delete [] r;
	}


}
