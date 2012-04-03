#include "ClothCL.h"

#define MSTRINGIFY(A) #A
static const char* PrepareLinksCLString = 
#include "OpenCLC10/PrepareLinks.cl"
static const char* UpdatePositionsFromVelocitiesCLString = 
#include "OpenCLC10/UpdatePositionsFromVelocities.cl"
static const char* SolvePositionsCLString = 
#include "OpenCLC10/SolvePositions.cl"
static const char* UpdateNodesCLString = 
#include "OpenCLC10/UpdateNodes.cl"
static const char* UpdatePositionsCLString = 
#include "OpenCLC10/UpdatePositions.cl"
static const char* UpdateConstantsCLString = 
#include "OpenCLC10/UpdateConstants.cl"
static const char* IntegrateCLString = 
#include "OpenCLC10/Integrate.cl"
static const char* ApplyForcesCLString = 
#include "OpenCLC10/ApplyForces.cl"
static const char* UpdateNormalsCLString = 
#include "OpenCLC10/UpdateNormals.cl"
static const char* VSolveLinksCLString = 
#include "OpenCLC10/VSolveLinks.cl"
static const char* SolveCollisionsAndUpdateVelocitiesCLString =
#include "OpenCLC10/SolveCollisionsAndUpdateVelocities.cl"

extern cl_context        g_cxGPUMainContext;
extern cl_command_queue  g_cqGPUCommandQue;

#define RELEASE_CL_KERNEL(kernelName) {if( kernelName ){ clReleaseKernel( kernelName ); kernelName = 0; }}

CClothCL::CClothCL(void) : m_bBuildCLKernels(false)
{
}

CClothCL::~CClothCL(void)
{
	ReleaseKernels();
}

void CClothCL::Initialize()
{
	m_clFunctions.m_cxMainContext = g_cxGPUMainContext;
	m_clFunctions.m_cqCommandQue = g_cqGPUCommandQue;
		
	BuildCLKernels();

	//m_VertexCLArray.reserve(m_VertexArray.size());
	int numVertices = (int)m_VertexArray.size();
	m_VertexCLArray = new VertexClothCL[numVertices];

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		VertexClothCL vertexData;

		vertexData.m_Index = m_VertexArray[i].m_Index;
		vertexData.m_InvMass = m_VertexArray[i].m_InvMass;
		vertexData.m_PinIndex = m_VertexArray[i].m_PinIndex;	
		
		vertexData.m_Pos = ToFloat4s(m_VertexArray[i].m_Pos.m_X, m_VertexArray[i].m_Pos.m_Y, m_VertexArray[i].m_Pos.m_Z);
		vertexData.m_Vel = ToFloat4s(m_VertexArray[i].m_Vel.m_X, m_VertexArray[i].m_Vel.m_Y, m_VertexArray[i].m_Vel.m_Z);
		vertexData.m_Accel = ToFloat4s(m_VertexArray[i].m_Accel.m_X, m_VertexArray[i].m_Accel.m_Y, m_VertexArray[i].m_Accel.m_Z);

		//m_VertexCLArray.push_back(vertexData);
		m_VertexCLArray[i] = vertexData;

		assert(m_VertexCLArray[i].m_Pos.x == m_VertexArray[i].m_Pos.m_X);
		assert(m_VertexCLArray[i].m_Pos.y == m_VertexArray[i].m_Pos.m_Y);
		assert(m_VertexCLArray[i].m_Pos.z == m_VertexArray[i].m_Pos.m_Z);
	}

	m_CLMemVertexArray = clCreateBuffer(g_cxGPUMainContext, CL_MEM_READ_WRITE, sizeof(VertexClothCL) * numVertices, NULL, NULL);
	cl_int result = clEnqueueWriteBuffer(g_cqGPUCommandQue, m_CLMemVertexArray, CL_TRUE, 0, sizeof(VertexClothCL) * numVertices, m_VertexCLArray, 0, NULL, NULL);

	assert(result == CL_SUCCESS);

	for ( int i = 0; i < numVertices; i++ )
	{
		const VertexClothCL& vertexData = m_VertexCLArray[i];

		m_VertexArray[i].m_Index = vertexData.m_Index;
		m_VertexArray[i].m_InvMass = vertexData.m_InvMass;
		m_VertexArray[i].m_PinIndex = vertexData.m_PinIndex;

		m_VertexArray[i].m_Pos.Set(vertexData.m_Pos.x, vertexData.m_Pos.y, vertexData.m_Pos.z);
	}

}

void CClothCL::ReleaseKernels()
{
	if ( !m_bBuildCLKernels )
		return;

	RELEASE_CL_KERNEL( m_prepareLinksKernel );
	RELEASE_CL_KERNEL( m_solvePositionsFromLinksKernel );
	RELEASE_CL_KERNEL( m_updateConstantsKernel );
	RELEASE_CL_KERNEL( m_integrateKernel );
	RELEASE_CL_KERNEL( m_addVelocityKernel );
	RELEASE_CL_KERNEL( m_updatePositionsFromVelocitiesKernel );
	RELEASE_CL_KERNEL( m_updateVelocitiesFromPositionsWithoutVelocitiesKernel );
	RELEASE_CL_KERNEL( m_updateVelocitiesFromPositionsWithVelocitiesKernel );
	RELEASE_CL_KERNEL( m_vSolveLinksKernel );
	RELEASE_CL_KERNEL( m_solveCollisionsAndUpdateVelocitiesKernel );
	RELEASE_CL_KERNEL( m_resetNormalsAndAreasKernel );
	RELEASE_CL_KERNEL( m_normalizeNormalsAndAreasKernel );
	RELEASE_CL_KERNEL( m_outputToVertexArrayKernel );
	RELEASE_CL_KERNEL( m_applyForcesKernel );
}

bool CClothCL::BuildCLKernels()
{
	if ( m_bBuildCLKernels )
		return true;

	ReleaseKernels();

	m_clFunctions.clearKernelCompilationFailures();

	m_applyForcesKernel = m_clFunctions.compileCLKernelFromString( ApplyForcesCLString, "ApplyForcesKernel" );

	/*
	m_prepareLinksKernel = m_clFunctions.compileCLKernelFromString( PrepareLinksCLString, "PrepareLinksKernel" );
	m_updatePositionsFromVelocitiesKernel = m_clFunctions.compileCLKernelFromString( UpdatePositionsFromVelocitiesCLString, "UpdatePositionsFromVelocitiesKernel" );
	m_solvePositionsFromLinksKernel = m_clFunctions.compileCLKernelFromString( SolvePositionsCLString, "SolvePositionsFromLinksKernel" );
	m_vSolveLinksKernel = m_clFunctions.compileCLKernelFromString( VSolveLinksCLString, "VSolveLinksKernel" );
	m_updateVelocitiesFromPositionsWithVelocitiesKernel = m_clFunctions.compileCLKernelFromString( UpdateNodesCLString, "updateVelocitiesFromPositionsWithVelocitiesKernel" );
	m_updateVelocitiesFromPositionsWithoutVelocitiesKernel = m_clFunctions.compileCLKernelFromString( UpdatePositionsCLString, "updateVelocitiesFromPositionsWithoutVelocitiesKernel" );
	m_solveCollisionsAndUpdateVelocitiesKernel = m_clFunctions.compileCLKernelFromString( SolveCollisionsAndUpdateVelocitiesCLString, "SolveCollisionsAndUpdateVelocitiesKernel" );
	m_integrateKernel = m_clFunctions.compileCLKernelFromString( IntegrateCLString, "IntegrateKernel" );
	
	m_resetNormalsAndAreasKernel = m_clFunctions.compileCLKernelFromString( UpdateNormalsCLString, "ResetNormalsAndAreasKernel" );
	m_normalizeNormalsAndAreasKernel = m_clFunctions.compileCLKernelFromString( UpdateNormalsCLString, "NormalizeNormalsAndAreasKernel" );
	m_updateSoftBodiesKernel = m_clFunctions.compileCLKernelFromString( UpdateNormalsCLString, "UpdateSoftBodiesKernel" );
	*/
	
	if( m_clFunctions.getKernelCompilationFailures()==0 )
		m_bBuildCLKernels = true;

	return m_bBuildCLKernels;
}

void CClothCL::IntegrateByLocalPositionContraints(btScalar dt)
{
	//cl_int ciErrNum ;
	int numVertices = (int)m_VertexArray.size();
	//ciErrNum = clSetKernelArg(m_applyForcesKernel, 0, sizeof(int), &numVertices);
	//ciErrNum = clSetKernelArg(m_applyForcesKernel, 1, sizeof(float), &dt);
	//ciErrNum = clSetKernelArg(m_applyForcesKernel, 2, sizeof(cl_mem), &m_CLMemVertexArray);

	//assert(ciErrNum == CL_SUCCESS);
	//	
	//size_t m_defaultWorkGroupSize = 64;
	//size_t numWorkItems = m_defaultWorkGroupSize*((numVertices + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

	///*ciErrNum = clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_applyForcesKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);
	//clFinish(g_cqGPUCommandQue);*/

	//assert(ciErrNum == CL_SUCCESS);

	//// Read data back to CPU
	////clEnqueueReadBuffer(g_cqGPUCommandQue, m_CLMemVertexArray, CL_TRUE, 0, sizeof(VertexClothCL) * numVertices, m_VertexCLArray, 0, NULL, NULL);

	for ( int i = 0; i < numVertices; i++ )
	{
		const VertexClothCL& vertexData = m_VertexCLArray[i];

		m_VertexArray[i].m_Index = vertexData.m_Index;
		m_VertexArray[i].m_InvMass = vertexData.m_InvMass;
		m_VertexArray[i].m_PinIndex = vertexData.m_PinIndex;

		m_VertexArray[i].m_Pos.Set(vertexData.m_Pos.x, vertexData.m_Pos.y, vertexData.m_Pos.z);
	}
}

void CClothCL::IntegrateEuler(btScalar dt)
{

}

void CClothCL::AdvancePosition(btScalar dt)
{

}