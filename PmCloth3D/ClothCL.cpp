#include <omp.h>

#ifdef WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
#  undef NOMINMAX
#endif

#include <GL/gl.h>

#include "ClothCL.h"

#define MSTRINGIFY(A) #A

static const char* ApplyForcesCLString = 
#include "OpenCLC10/ApplyForces.cl"

extern cl_context        g_cxGPUMainContext;
extern cl_command_queue  g_cqGPUCommandQue;

#define RELEASE_CL_KERNEL(kernelName) {if( kernelName ){ clReleaseKernel( kernelName ); kernelName = 0; }}

CClothCL::CClothCL(void) : m_bBuildCLKernels(false), m_HBVertexCL(NULL), m_HBSpringCL(NULL), m_DBVertices(NULL), m_DBStrechSprings(NULL)
{
	m_ClearForcesKernel = NULL;
	m_ComputeNextVertexPositionsKernel = NULL;
	m_ApplyGravityKernel = NULL;
	m_ApplyForcesKernel = NULL;
	m_EnforceEdgeConstraintsKernel = NULL;
	m_UpdateVelocitiesKernel = NULL;
	m_AdvancePositionKernel = NULL;
}

CClothCL::~CClothCL(void)
{
	if ( m_DBVertices )
		clReleaseMemObject(m_DBVertices);
	
	if ( m_DBStrechSprings )
		clReleaseMemObject(m_DBStrechSprings);

	ReleaseKernels();	
	
	if ( m_HBVertexCL )
		delete m_HBVertexCL;

	if ( m_HBSpringCL )
		delete m_HBSpringCL;
}

void CClothCL::Initialize()
{
	CCloth::Initialize();

	m_clFunctions.m_cxMainContext = g_cxGPUMainContext;
	m_clFunctions.m_cqCommandQue = g_cqGPUCommandQue;
		
	BuildCLKernels();

	//---------------------
	// Buffer for vertices
	//---------------------
	int numVertices = (int)m_VertexArray.size();	
	m_HBVertexCL = new VertexClothCL[numVertices];
	m_DBVertices = clCreateBuffer(g_cxGPUMainContext, CL_MEM_READ_WRITE, sizeof(VertexClothCL) * numVertices, NULL, NULL);

	//----------------------------
	// Buffer for stretch springs
	//----------------------------
	int numSprings = (int)m_StrechSpringArray.size();
	m_HBSpringCL = new SpringClothCL[numSprings];	
	m_DBStrechSprings = clCreateBuffer(g_cxGPUMainContext, CL_MEM_READ_WRITE, sizeof(SpringClothCL) * numSprings, NULL, NULL);
}

void CClothCL::UpdateBuffers()
{
	int numVertices = (int)m_VertexArray.size();
	int numSprings = (int)m_StrechSpringArray.size();

	//---------------------
	// Buffer for vertices
	//---------------------
	for ( int i = 0; i < numVertices; i++ )
	{
		VertexClothCL vertexData;
		
		vertexData.m_Pos = ToFloat4s(m_VertexArray[i].m_Pos);
		vertexData.m_Vel = ToFloat4s(m_VertexArray[i].m_Vel);
		vertexData.m_Accel = ToFloat4s(m_VertexArray[i].m_Accel);

		m_HBVertexCL[i] = vertexData;
	}	
	
	cl_int result = clEnqueueWriteBuffer(g_cqGPUCommandQue, m_DBVertices, CL_TRUE, 0, sizeof(VertexClothCL) * numVertices, m_HBVertexCL, 0, NULL, NULL);
	assert(result == CL_SUCCESS);	

	//----------------------------
	// Buffer for stretch springs
	//----------------------------
	for ( int i = 0; i < numSprings; i++ )
	{
		const CSpringCloth3D& springData = m_StrechSpringArray[i];

		m_HBSpringCL[i].m_Index = springData.GetIndex();
		m_HBSpringCL[i].m_IndexVrx0 = springData.GetVertexIndex(0);
		m_HBSpringCL[i].m_IndexVrx1 = springData.GetVertexIndex(1);
		m_HBSpringCL[i].m_RestLength = springData.GetRestLength();
	}
	
	result = clEnqueueWriteBuffer(g_cqGPUCommandQue, m_DBStrechSprings, CL_TRUE, 0, sizeof(SpringClothCL) * numSprings, m_HBSpringCL, 0, NULL, NULL);
	assert(result == CL_SUCCESS);
}

bool CClothCL::Integrate(btScalar dt)
{
	int numVertices = (int)m_VertexArray.size();
	int numSprings = (int)m_StrechSpringArray.size();

	if ( numVertices == 0 )
		return true;

	UpdateBuffers();

	//-------------------
	// ClearForcesKernel
	//-------------------
	{
		cl_int ciErrNum;	
		ciErrNum = clSetKernelArg(m_ClearForcesKernel, 0, sizeof(unsigned int), &numVertices);
		ciErrNum = clSetKernelArg(m_ClearForcesKernel, 1, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_ClearForcesKernel, 2, sizeof(cl_mem), &m_DBVertices);

		assert(ciErrNum == CL_SUCCESS);
		
		size_t m_defaultWorkGroupSize = 64;
		size_t numWorkItems = m_defaultWorkGroupSize*((numVertices + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_ClearForcesKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);
	}

	//-------------------
	// ApplyGravityKernel
	//-------------------
	{
		cl_int ciErrNum;	
		ciErrNum = clSetKernelArg(m_ApplyGravityKernel, 0, sizeof(unsigned int), &numVertices);
		ciErrNum = clSetKernelArg(m_ApplyGravityKernel, 1, sizeof(float4), &ToFloat4s(m_Gravity));
		ciErrNum = clSetKernelArg(m_ApplyGravityKernel, 2, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_ApplyGravityKernel, 3, sizeof(cl_mem), &m_DBVertices);

		assert(ciErrNum == CL_SUCCESS);
		
		size_t m_defaultWorkGroupSize = 64;
		size_t numWorkItems = m_defaultWorkGroupSize*((numVertices + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_ApplyGravityKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);
	}

	//------------------
	// ApplyForcesKernel
	//------------------
	{
		cl_int ciErrNum;	
		ciErrNum = clSetKernelArg(m_ApplyForcesKernel, 0, sizeof(unsigned int), &numVertices);
		ciErrNum = clSetKernelArg(m_ApplyForcesKernel, 1, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_ApplyForcesKernel, 2, sizeof(cl_mem), &m_DBVertices);

		assert(ciErrNum == CL_SUCCESS);
		
		size_t m_defaultWorkGroupSize = 64;
		size_t numWorkItems = m_defaultWorkGroupSize*((numVertices + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_ApplyForcesKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);
	}

	//-------------------
	// ClearForcesKernel
	//-------------------
	{
		cl_int ciErrNum;	
		ciErrNum = clSetKernelArg(m_ClearForcesKernel, 0, sizeof(unsigned int), &numVertices);
		ciErrNum = clSetKernelArg(m_ClearForcesKernel, 1, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_ClearForcesKernel, 2, sizeof(cl_mem), &m_DBVertices);

		assert(ciErrNum == CL_SUCCESS);
		
		size_t m_defaultWorkGroupSize = 64;
		size_t numWorkItems = m_defaultWorkGroupSize*((numVertices + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_ClearForcesKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);
	}

	//----------------------------------
	// ComputeNextVertexPositionsKernel
	//----------------------------------
	{
		cl_int ciErrNum;	
		ciErrNum = clSetKernelArg(m_ComputeNextVertexPositionsKernel, 0, sizeof(unsigned int), &numVertices);
		ciErrNum = clSetKernelArg(m_ComputeNextVertexPositionsKernel, 1, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_ComputeNextVertexPositionsKernel, 2, sizeof(cl_mem), &m_DBVertices);

		assert(ciErrNum == CL_SUCCESS);
		
		size_t m_defaultWorkGroupSize = 64;
		size_t numWorkItems = m_defaultWorkGroupSize*((numVertices + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_ComputeNextVertexPositionsKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);
	}

	//-------------------------------
	// EnforceEdgeConstraintsKernel
	//-------------------------------
	for ( int batch = 0; batch < (int)m_BatchSpringIndexArray.size()-1; batch++ )
	{
		int startSpringIndex = m_BatchSpringIndexArray[batch];
		int endSpringIndex = m_BatchSpringIndexArray[batch+1]-1;

		int numSpringsInBatch = endSpringIndex - startSpringIndex + 1;

		cl_int ciErrNum;	
		ciErrNum = clSetKernelArg(m_EnforceEdgeConstraintsKernel, 0, sizeof(unsigned int), &numSpringsInBatch);
		ciErrNum = clSetKernelArg(m_EnforceEdgeConstraintsKernel, 1, sizeof(unsigned int), &startSpringIndex);
		ciErrNum = clSetKernelArg(m_EnforceEdgeConstraintsKernel, 2, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_EnforceEdgeConstraintsKernel, 3, sizeof(cl_mem), &m_DBVertices);
		ciErrNum = clSetKernelArg(m_EnforceEdgeConstraintsKernel, 4, sizeof(cl_mem), &m_DBStrechSprings);
		
		assert(ciErrNum == CL_SUCCESS);
		
		size_t m_defaultWorkGroupSize = 64;
		size_t numWorkItems = m_defaultWorkGroupSize*((numSpringsInBatch + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_EnforceEdgeConstraintsKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);
	}

	//-----------------------
	// UpdateVelocitiesKernel
	//-----------------------
	{
		cl_int ciErrNum;	
		ciErrNum = clSetKernelArg(m_UpdateVelocitiesKernel, 0, sizeof(unsigned int), &numVertices);
		ciErrNum = clSetKernelArg(m_UpdateVelocitiesKernel, 1, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_UpdateVelocitiesKernel, 2, sizeof(cl_mem), &m_DBVertices);

		assert(ciErrNum == CL_SUCCESS);
		
		size_t m_defaultWorkGroupSize = 64;
		size_t numWorkItems = m_defaultWorkGroupSize*((numVertices + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_UpdateVelocitiesKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);
	}

	//------------------------
	// Read data back to CPU
	//------------------------
	clFinish(g_cqGPUCommandQue);
	cl_int ciErrNum = clEnqueueReadBuffer(g_cqGPUCommandQue, m_DBVertices, CL_TRUE, 0, sizeof(VertexClothCL) * numVertices, m_HBVertexCL, 0, NULL, NULL);

	// TODO: if though ciErrNum is 0, the following line doesn't work. 
	/*if (ciErrNum != CL_SUCCESS);
		return false;*/

	for ( int i = 0; i < numVertices; i++ )
	{
		const VertexClothCL& vertexData = m_HBVertexCL[i];
		m_VertexArray[i].m_Pos.Set(vertexData.m_Pos.x, vertexData.m_Pos.y, vertexData.m_Pos.z);
		m_VertexArray[i].m_PosNext.Set(vertexData.m_PosNext.x, vertexData.m_PosNext.y, vertexData.m_PosNext.z);
		m_VertexArray[i].m_Vel.Set(vertexData.m_Vel.x, vertexData.m_Vel.y, vertexData.m_Vel.z);
	}

	return true;
}

bool CClothCL::AdvancePosition(btScalar dt)
{
	UpdateBuffers();

	int numVertices = (int)m_VertexArray.size();

	//-----------------------
	// AdvancePositionKernel
	//-----------------------
	{
		cl_int ciErrNum;	
		ciErrNum = clSetKernelArg(m_AdvancePositionKernel, 0, sizeof(unsigned int), &numVertices);
		ciErrNum = clSetKernelArg(m_AdvancePositionKernel, 1, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_AdvancePositionKernel, 2, sizeof(cl_mem), &m_DBVertices);

		assert(ciErrNum == CL_SUCCESS);
		
		size_t m_defaultWorkGroupSize = 64;
		size_t numWorkItems = m_defaultWorkGroupSize*((numVertices + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_AdvancePositionKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);
	}

	//------------------------
	// Read data back to CPU
	//------------------------
	clFinish(g_cqGPUCommandQue);
	cl_int ciErrNum = clEnqueueReadBuffer(g_cqGPUCommandQue, m_DBVertices, CL_TRUE, 0, sizeof(VertexClothCL) * numVertices, m_HBVertexCL, 0, NULL, NULL);

	// TODO: if though ciErrNum is 0, the following line doesn't work. 
	/*if (ciErrNum != CL_SUCCESS);
		return false;*/

	for ( int i = 0; i < numVertices; i++ )
	{
		const VertexClothCL& vertexData = m_HBVertexCL[i];
		m_VertexArray[i].m_Pos.Set(vertexData.m_Pos.x, vertexData.m_Pos.y, vertexData.m_Pos.z);
		m_VertexArray[i].m_PosNext.Set(vertexData.m_PosNext.x, vertexData.m_PosNext.y, vertexData.m_PosNext.z);
		m_VertexArray[i].m_Vel.Set(vertexData.m_Vel.x, vertexData.m_Vel.y, vertexData.m_Vel.z);
	}
	
	return true;
}

bool CClothCL::ResolveCollision(CCollisionObject& convexObject, btScalar dt)
{
	return CCloth::ResolveCollision(convexObject, dt);
}

void CClothCL::ReleaseKernels()
{
	if ( !m_bBuildCLKernels )
		return;

	RELEASE_CL_KERNEL(m_ClearForcesKernel);
	RELEASE_CL_KERNEL(m_ComputeNextVertexPositionsKernel);	
	RELEASE_CL_KERNEL(m_ApplyGravityKernel);
	RELEASE_CL_KERNEL(m_ApplyForcesKernel);
	RELEASE_CL_KERNEL(m_EnforceEdgeConstraintsKernel);
	RELEASE_CL_KERNEL(m_UpdateVelocitiesKernel);
	RELEASE_CL_KERNEL(m_AdvancePositionKernel);
}

bool CClothCL::BuildCLKernels()
{
	if ( m_bBuildCLKernels )
		return true;

	ReleaseKernels();

	m_clFunctions.clearKernelCompilationFailures();

	m_ClearForcesKernel = m_clFunctions.compileCLKernelFromString(ApplyForcesCLString, "ClearForcesKernel");
	m_ComputeNextVertexPositionsKernel = m_clFunctions.compileCLKernelFromString(ApplyForcesCLString, "ComputeNextVertexPositionsKernel");
	m_ApplyGravityKernel = m_clFunctions.compileCLKernelFromString(ApplyForcesCLString, "ApplyGravityKernel");
	m_ApplyForcesKernel = m_clFunctions.compileCLKernelFromString(ApplyForcesCLString, "ApplyForcesKernel");
	m_EnforceEdgeConstraintsKernel = m_clFunctions.compileCLKernelFromString(ApplyForcesCLString, "EnforceEdgeConstraintsKernel");
	m_UpdateVelocitiesKernel = m_clFunctions.compileCLKernelFromString(ApplyForcesCLString, "UpdateVelocitiesKernel");
	m_AdvancePositionKernel = m_clFunctions.compileCLKernelFromString(ApplyForcesCLString, "AdvancePositionKernel");
	
	if( m_clFunctions.getKernelCompilationFailures()==0 )
		m_bBuildCLKernels = true;

	return m_bBuildCLKernels;
}


