#include <GraphicsTestBed/Demos/VectorMath.cl>
#include <GraphicsTestBed/Demos/UniformGridDefines.h>
#include <GraphicsTestBed/Demos/UniformGridFuncs.h>


typedef struct
{
	float4 m_g;
	int m_numParticles;
	float m_dt;
	float m_scale;
	float m_e;

	int4 m_nCells;
	float4 m_spaceMin;
	float m_gridScale;

}ConstBuffer;


__inline
float div(float a, float b)
{
	return native_divide(a,b);
}

__inline
float4 calcForce(const float4 x_i, const float4 x_j, const float4 v_i, const float4 v_j, 
				 float r_i, float r_j, float m_i, float m_j, float dt,
				 float e)
{
	float4 f = make_float4(0,0,0,0);
	float sCoeff, dCoeff;

	{
		float m = div(m_i*m_j,m_i+m_j);
		sCoeff = div(m,dt*dt);
		dCoeff = div(m*(1.f-e),dt);
	}

	float4 x_ij = x_j-x_i;
	float dist = native_sqrt( dot3F4(x_ij, x_ij) );

	if( dist < r_i+r_j )
	{
		f -= sCoeff*(r_i+r_j-dist)*x_ij*div(1.f,dist);
		f += dCoeff*(v_j - v_i);
	}
	return f;
}

__inline
float4 calcForce1(const float4 x_i, const float4 x_j, const float4 v_i, const float4 v_j, 
				 float r_i, float r_j, float m_i, float m_j, float dt,
				 float sCoeff, float dCoeff)
{
	float4 f = make_float4(0,0,0,0);
	float4 x_ij = x_j-x_i;
	float dist = native_sqrt( dot3F4(x_ij, x_ij) );

	if( dist < r_i+r_j )
	{
		f -= sCoeff*(r_i+r_j-dist)*x_ij*div(1.f,dist);
		f += dCoeff*(v_j - v_i);
	}
	return f;
}

__kernel
void CollideAllKernel(__global float4* posIn, __global float4* velIn, __global float4* forceOut,
					  ConstBuffer cb)
{
	if( GET_GLOBAL_IDX >= cb.m_numParticles ) return;

	float4 f = make_float4(0,0,0,0);

	float4 x_i = posIn[ GET_GLOBAL_IDX ];
	float4 v_i = velIn[ GET_GLOBAL_IDX ];

	for(int j=0; j<cb.m_numParticles; j++)
	{
		if( j==GET_GLOBAL_IDX ) continue;

		float4 x_j = posIn[ j ];
		float4 v_j = velIn[ j ];

		f += calcForce( x_i, x_j, v_i, v_j, x_i.w, x_j.w, v_i.w, v_j.w, cb.m_dt, cb.m_e );
	}

	{
		float sCoeff, dCoeff;
		{
			float m = v_i.w/2.f;
			sCoeff = m/(cb.m_dt*cb.m_dt);
			dCoeff = m/cb.m_dt*(1.f-cb.m_e);
		}

		float4 planes[4];
		planes[0] = make_float4(0,1,0,cb.m_scale);
		planes[1] = make_float4(-1,0,0,cb.m_scale);
		planes[2] = make_float4(1,0,0,cb.m_scale);
		planes[3] = make_float4(0,-1,0,cb.m_scale);

		for(int j=0; j<4; j++)
		{
			float4 eqn = planes[j];
			float dist = dot3w1( x_i, eqn );
			float r_i = x_i.w;
			if( dist < r_i )
			{
				f += sCoeff*(r_i-dist)*eqn;
				f += dCoeff*(-v_i);
			}
		}
	}

	forceOut[ GET_GLOBAL_IDX ] = f;
}

__kernel
void CopyPosVelKernel(__global float4* posIn, __global float4* velIn,
						__global float4* posOut, __global float4* velOut,
						ConstBuffer cb)
{
	int gIdx = GET_GLOBAL_IDX;

	if( gIdx >= cb.m_numParticles ) return;

	posOut[gIdx] = posIn[gIdx];
	velOut[gIdx] = velIn[gIdx];
}

__kernel
void CollideGridKernel(__global float4* posIn, __global float4* velIn, 
					   __global int* cGrid, __global int* cGridCounter, 
					   __global float4* forceOut,
					   ConstBuffer cb)
{
	if( GET_GLOBAL_IDX >= cb.m_numParticles ) return;

	int4 nCells = cb.m_nCells;
	float4 spaceMin = cb.m_spaceMin;
	float gridScale = cb.m_gridScale;
	float dt = cb.m_dt;
	float e = cb.m_e;

	float4 f = make_float4(0,0,0,0);

	float4 x_i = posIn[ GET_GLOBAL_IDX ];
	float4 v_i = velIn[ GET_GLOBAL_IDX ];

	float sCoeff, dCoeff;
	{
		float m_i = v_i.w;
		float m_j = v_i.w;
		float m = div(m_i*m_j,m_i+m_j);
		sCoeff = div(m,dt*dt);
		dCoeff = div(m*(1.f-e),dt);
	}

	int4 iGridCrd = ugConvertToGridCrd( x_i-spaceMin, gridScale );

int k=0;
	for(int i=-1;i<=1;i++) for(int j=-1;j<=1;j++)
	{
		int4 gridCrd = make_int4(iGridCrd.x+i, iGridCrd.y+j, iGridCrd.z+k, 0);
		
		if( gridCrd.x < 0 || gridCrd.x >= nCells.x
			|| gridCrd.y < 0 || gridCrd.y >= nCells.y ) continue;
		
		int gridIdx = ugGridCrdToGridIdx( gridCrd, nCells.x, nCells.y, nCells.z );

		int numElem = cGridCounter[ gridIdx ];
		numElem = min(numElem, MAX_IDX_PER_GRID);

		for(int ie=0; ie<numElem; ie++)
		{
			int jIdx = cGrid[ MAX_IDX_PER_GRID*gridIdx + ie ];
			if( jIdx == GET_GLOBAL_IDX ) continue;
			
			float4 x_j = posIn[jIdx];
			float4 v_j = velIn[jIdx];

			f += calcForce1( x_i, x_j, v_i, v_j, x_i.w, x_j.w, v_i.w, v_j.w, dt, sCoeff, dCoeff );
		}
	}

	{
		float sCoeff, dCoeff;
		{
			float m = v_i.w/2.f;
			sCoeff = div(m,dt*dt);
			dCoeff = div(m*(1.f-e),dt);
		}

		float4 planes[4];
		planes[0] = make_float4(0,1,0,cb.m_scale);
		planes[1] = make_float4(-1,0,0,cb.m_scale);
		planes[2] = make_float4(1,0,0,cb.m_scale);
		planes[3] = make_float4(0,-1,0,cb.m_scale);

		for(int j=0; j<4; j++)
		{
			float4 eqn = planes[j];
			float dist = dot3w1( x_i, eqn );
			float r_i = x_i.w;
			if( dist < r_i )
			{
				f += sCoeff*(r_i-dist)*eqn;
				f += dCoeff*(-v_i);
			}
		}
	}

	forceOut[ GET_GLOBAL_IDX ] = f;
}

__kernel
void IntegrateKernel(__global float4* pos, __global float4* vel, __global float4* force, __global float4* forceInt,
					 ConstBuffer cb)
{
	if( GET_GLOBAL_IDX >= cb.m_numParticles ) return;

	float4 x = pos[GET_GLOBAL_IDX];
	float4 v = vel[GET_GLOBAL_IDX];

	v += (force[GET_GLOBAL_IDX]+forceInt[GET_GLOBAL_IDX])*cb.m_dt/v.w+cb.m_g;
	x += v*cb.m_dt;

	pos[GET_GLOBAL_IDX] = make_float4(x.x, x.y, x.z, pos[GET_GLOBAL_IDX].w);
	vel[GET_GLOBAL_IDX] = make_float4(v.x, v.y, v.z, vel[GET_GLOBAL_IDX].w);
	//f0[i] = make_float4(0,0,0,0);
	forceInt[GET_GLOBAL_IDX] = make_float4(0,0,0,0);
}
