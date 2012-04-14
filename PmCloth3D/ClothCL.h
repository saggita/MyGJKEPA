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

	/*SpringClothCL() 
	{ 
		m_IndexVrx0 = -1; 
		m_IndexVrx1 = -1; 
		m_IndexTriangle0 = -1;
		m_IndexTriangle1 = -1;
		m_Index = -1;
	}

	SpringClothCL(int indexVrx0, int indexVrx1) 
	{ 
		m_IndexVrx0 = indexVrx0; 
		m_IndexVrx1 = indexVrx1; 
		m_IndexTriangle0 = -1;
		m_IndexTriangle1 = -1;
		m_Index = -1;
	}

	~SpringClothCL() {}*/

	unsigned int m_Index;
	unsigned int m_IndexVrx0;
	unsigned int m_IndexVrx1;
	unsigned int m_IndexTriangle0;
	unsigned int m_IndexTriangle1;
	float m_RestLength;
};

_MEM_CLASSALIGN16
struct VertexClothCL
{
	_MEM_ALIGNED_ALLOCATOR16;
	
	float4s m_Pos;
	float4s m_PosTemp;
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
	std::vector<int> m_BatchSpringIndexArray;

	bool m_bBuildCLKernels;

	bool BuildCLKernels();
	void ReleaseKernels();
	void GenerateBatches();

	// OpenCL kernels
	/*cl_kernel m_prepareLinksKernel;
	cl_kernel m_solvePositionsFromLinksKernel;
	cl_kernel m_updateConstantsKernel;
	cl_kernel m_integrateKernel;
	cl_kernel m_addVelocityKernel;
	cl_kernel m_updatePositionsFromVelocitiesKernel;
	cl_kernel m_updateVelocitiesFromPositionsWithoutVelocitiesKernel;
	cl_kernel m_updateVelocitiesFromPositionsWithVelocitiesKernel;
	cl_kernel m_vSolveLinksKernel;
	cl_kernel m_solveCollisionsAndUpdateVelocitiesKernel;
	cl_kernel m_resetNormalsAndAreasKernel;
	cl_kernel m_normalizeNormalsAndAreasKernel;
	cl_kernel m_updateSoftBodiesKernel;
	cl_kernel m_outputToVertexArrayKernel;*/

	cl_kernel m_applyGravityKernel;
	cl_kernel m_applyForcesKernel;
	cl_kernel m_integrateKernel;

	CLFunctions m_clFunctions;

public:
	virtual void Initialize();
	virtual bool Integrate(btScalar dt);
	virtual bool AdvancePosition(btScalar dt);
};

