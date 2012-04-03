#pragma once

#include <CL\cl.h>
#include "Cloth.h"
#include "CLFunctions.h"
#include "Adl\AdlPrimitives\Math\Math.h"

struct float4s
{
	union
	{
		struct
		{
			float x,y,z,w;
		};
		struct
		{
			float s[4];
		};
	};
};

__inline
float4s ToFloat4s(float x, float y, float z, float w = 0.f)
{
	float4s v;
	v.x = x; v.y = y; v.z = z; v.w = w;
	return v;
}

struct SpringClothCL
{	
public:
	SpringClothCL() 
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

	~SpringClothCL() {}

	int m_Index;
	int m_IndexVrx0;
	int m_IndexVrx1;
	int m_IndexTriangle0;
	int m_IndexTriangle1;
	float m_RestLength;
};

struct VertexClothCL
{
public:
	VertexClothCL() : m_InvMass(1.0), m_PinIndex(-1)
	{
	}

	~VertexClothCL() {};

	int m_Index;
	float m_InvMass;
	float4s m_Pos;
	float4s m_Vel;
	float4s m_Accel;
	int m_PinIndex;
};

class CClothCL : public CCloth
{
public:
	CClothCL(void);
	virtual ~CClothCL(void);

protected:
	cl_mem m_CLMemVertexArray;
	cl_mem m_CLMemStrechSpringArray;
	cl_mem m_CLMemBendSpringArray;

	VertexClothCL* m_VertexCLArray;

	bool m_bBuildCLKernels;

	bool BuildCLKernels();
	void ReleaseKernels();

	// OpenCL kernels
	cl_kernel m_prepareLinksKernel;
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
	cl_kernel m_outputToVertexArrayKernel;
	cl_kernel m_applyForcesKernel;

	CLFunctions m_clFunctions;

public:
	virtual void Initialize();

	virtual void IntegrateByLocalPositionContraints(btScalar dt);
	virtual void IntegrateEuler(btScalar dt);
	virtual void AdvancePosition(btScalar dt);
};

