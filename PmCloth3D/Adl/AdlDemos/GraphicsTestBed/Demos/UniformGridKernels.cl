#include <GraphicsTestBed/Demos/UniformGridDefines.h>
#include <GraphicsTestBed/Demos/UniformGridFuncs.h>

#define u32 uint
#define GET_GLOBAL_IDX get_global_id(0)

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable


typedef struct
{
	float4 m_max;
	float4 m_min;
	int4 m_nCells;
	float m_gridScale;
	u32 m_maxParticles;
} ConstBuffer;


__kernel
void GridConstructionKernel( __global float4* gPosIn, __global int* gridG, __global int* gridCounterG,
							ConstBuffer cb )
{
	if( GET_GLOBAL_IDX >= cb.m_maxParticles ) return;

	float4 iPos = gPosIn[GET_GLOBAL_IDX];

	int4 gridCrd = ugConvertToGridCrd( iPos-cb.m_min, cb.m_gridScale );

	if( gridCrd.x < 0 || gridCrd.x >= cb.m_nCells.x 
		|| gridCrd.y < 0 || gridCrd.y >= cb.m_nCells.y
		|| gridCrd.z < 0 || gridCrd.z >= cb.m_nCells.z ) return;
	
	int gridIdx = ugGridCrdToGridIdx( gridCrd, cb.m_nCells.x, cb.m_nCells.y, cb.m_nCells.z );

	int count = atom_add(&gridCounterG[gridIdx], 1);

	if( count < MAX_IDX_PER_GRID )
	{
		gridG[ gridIdx*MAX_IDX_PER_GRID + count ] = GET_GLOBAL_IDX;
	}
}

__kernel
void GridClearKernel( __global int* gridCounterC, ConstBuffer cb )
{
	int4 m_nCells = cb.m_nCells;
	if( GET_GLOBAL_IDX >= m_nCells.x*m_nCells.y*m_nCells.z ) return;
	gridCounterC[ GET_GLOBAL_IDX ] = 0;
}
