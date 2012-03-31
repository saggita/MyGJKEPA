#include "ClothCL.h"

extern cl_context        g_cxGPUMainContext;
extern cl_command_queue  g_cqGPUCommandQue;

CClothCL::CClothCL(void)
{
}

CClothCL::~CClothCL(void)
{
}

void CClothCL::Initialize()
{
	m_VertexArrayCL = clCreateBuffer(g_cxGPUMainContext, CL_MEM_READ_WRITE, sizeof(CVertexCloth3D) * m_VertexArray.size(), NULL, NULL);

	cl_int result = clEnqueueWriteBuffer(g_cqGPUCommandQue, m_VertexArrayCL, CL_TRUE, 0, sizeof(CVertexCloth3D) * m_VertexArray.size(), &m_VertexArray[0], 0, NULL, NULL);

	assert(result == CL_SUCCESS);
}

void CClothCL::IntegrateByLocalPositionContraints(btScalar dt)
{

}

void CClothCL::IntegrateEuler(btScalar dt)
{

}

void CClothCL::AdvancePosition(btScalar dt)
{

}