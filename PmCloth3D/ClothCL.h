#pragma once

#include <CL\cl.h>
#include "Cloth.h"
#include "CLFunctions.h"
#include "Adl\AdlPrimitives\Math\Math.h"

struct float4s
{	
	float x,y,z,w;		
};

__inline
float4s ToFloat4s(float x, float y, float z, float w = 0.f)
{
	float4s v;
	v.x = x; v.y = y; v.z = z; v.w = w;
	return v;
}

__inline
float4s ToFloat4s(const CVector3D& vec)
{
	float4s v;
	v.x = vec.m_X; v.y = vec.m_Y; v.z = vec.m_Z; v.w = 0.f;
	return v;
}

_MEM_CLASSALIGN16
struct SpringClothCL
{	
	_MEM_ALIGNED_ALLOCATOR16;

	unsigned int m_Index;
	unsigned int m_IndexVrx0;
	unsigned int m_IndexVrx1;
	float m_RestLength;
};

_MEM_CLASSALIGN16
struct VertexClothCL
{
	_MEM_ALIGNED_ALLOCATOR16;
	
	float4s m_Pos;
	float4s m_PosNext;
	float4s m_Vel;
	float4s m_Accel;
	
	float m_InvMass;
	unsigned int m_Index;
	unsigned int m_PinIndex;
};

class CClothCL : public CCloth
{
public:
	CClothCL(void);
	virtual ~CClothCL(void);

protected:
	cl_mem m_DBVertices;
	cl_mem m_DBStrechSprings;
	//cl_mem m_DBBendSprings;

	VertexClothCL* m_HBVertexCL;
	SpringClothCL* m_HBSpringCL;
	
	bool m_bBuildCLKernels;
	bool BuildCLKernels();
	void ReleaseKernels();	
	void UpdateBuffers();

	// OpenCL kernels
	cl_kernel m_ClearForcesKernel;
	cl_kernel m_ComputeNextVertexPositionsKernel;
	cl_kernel m_ApplyGravityKernel;
	cl_kernel m_ApplyForcesKernel;
	cl_kernel m_EnforceEdgeConstraintsKernel;
	cl_kernel m_UpdateVelocitiesKernel;
	cl_kernel m_AdvancePositionKernel;

	CLFunctions m_clFunctions;

public:
	virtual void Initialize();
	virtual bool Integrate(float dt);
	virtual bool AdvancePosition(float dt);
	virtual bool ResolveCollision(CCollisionObject& convexObject, float dt);
};

