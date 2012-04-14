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

CClothCL::CClothCL(void) : m_bBuildCLKernels(false), m_HBVertexCL(NULL), m_HBSpringCL(NULL)
{
	m_applyGravityKernel = NULL;
	m_applyForcesKernel = NULL;
	m_integrateKernel = NULL;
}

CClothCL::~CClothCL(void)
{
	clReleaseMemObject(m_DBVertices);
	clReleaseMemObject(m_DBStrechSprings);

	ReleaseKernels();	
	
	if ( m_HBVertexCL )
		delete m_HBVertexCL;

	if ( m_HBSpringCL )
		delete m_HBSpringCL;
}

void CClothCL::Initialize()
{
	m_clFunctions.m_cxMainContext = g_cxGPUMainContext;
	m_clFunctions.m_cqCommandQue = g_cqGPUCommandQue;
		
	BuildCLKernels();

	//---------------------
	// Buffer for vertices
	//---------------------

	//m_HBVertexCL.reserve(m_VertexArray.size());
	int numVertices = (int)m_VertexArray.size();
	
	m_HBVertexCL = new VertexClothCL[numVertices];

	for ( int i = 0; i < (int)m_VertexArray.size(); i++ )
	{
		VertexClothCL vertexData;

		/*vertexData.m_Index = m_VertexArray[i].m_Index;
		vertexData.m_InvMass = m_VertexArray[i].m_InvMass;
		vertexData.m_PinIndex = m_VertexArray[i].m_PinIndex;	*/
		
		vertexData.m_Pos = ToFloat4s(m_VertexArray[i].m_Pos);
		/*vertexData.m_Vel = ToFloat4s(m_VertexArray[i].m_Vel.m_X, m_VertexArray[i].m_Vel.m_Y, m_VertexArray[i].m_Vel.m_Z);
		vertexData.m_Accel = ToFloat4s(m_VertexArray[i].m_Accel.m_X, m_VertexArray[i].m_Accel.m_Y, m_VertexArray[i].m_Accel.m_Z);
*/
		//m_HBVertexCL.push_back(vertexData);
		m_HBVertexCL[i] = vertexData;

		assert(m_HBVertexCL[i].m_Pos.x == m_VertexArray[i].m_Pos.m_X);
		assert(m_HBVertexCL[i].m_Pos.y == m_VertexArray[i].m_Pos.m_Y);
		assert(m_HBVertexCL[i].m_Pos.z == m_VertexArray[i].m_Pos.m_Z);
	}

	m_DBVertices = clCreateBuffer(g_cxGPUMainContext, CL_MEM_READ_WRITE, sizeof(VertexClothCL) * numVertices, NULL, NULL);
	cl_int result = clEnqueueWriteBuffer(g_cqGPUCommandQue, m_DBVertices, CL_TRUE, 0, sizeof(VertexClothCL) * numVertices, m_HBVertexCL, 0, NULL, NULL);
	assert(result == CL_SUCCESS);	

	//----------------------------
	// Buffer for stretch springs
	//----------------------------
	int numSprings = (int)m_StrechSpringArray.size();

	m_HBSpringCL = new SpringClothCL[numSprings];

	for ( int i = 0; i < numSprings; i++ )
	{
		const CSpringCloth3D& springData = m_StrechSpringArray[i];

		m_HBSpringCL[i].m_Index = springData.GetIndex();
		m_HBSpringCL[i].m_IndexVrx0 = springData.GetVertexIndex(0);
		m_HBSpringCL[i].m_IndexVrx1 = springData.GetVertexIndex(1);
		m_HBSpringCL[i].m_RestLength = springData.GetRestLength();
	}

	m_DBStrechSprings = clCreateBuffer(g_cxGPUMainContext, CL_MEM_READ_WRITE, sizeof(SpringClothCL) * numSprings, NULL, NULL);
	result = clEnqueueWriteBuffer(g_cqGPUCommandQue, m_DBStrechSprings, CL_TRUE, 0, sizeof(SpringClothCL) * numSprings, m_HBSpringCL, 0, NULL, NULL);
	assert(result == CL_SUCCESS);

	GenerateBatches();
}

bool CClothCL::Integrate(btScalar dt)
{
	int numVertices = (int)m_VertexArray.size();
	int numSprings = (int)m_StrechSpringArray.size();

	//------------------
	// ApplyGravityKernel
	//------------------
	{
		cl_int ciErrNum;	
		ciErrNum = clSetKernelArg(m_applyGravityKernel, 0, sizeof(int), &numVertices);
		ciErrNum = clSetKernelArg(m_applyGravityKernel, 1, sizeof(float4), &ToFloat4s(m_Gravity));
		ciErrNum = clSetKernelArg(m_applyGravityKernel, 2, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_applyGravityKernel, 3, sizeof(cl_mem), &m_DBVertices);

		assert(ciErrNum == CL_SUCCESS);
		
		size_t m_defaultWorkGroupSize = 64;
		size_t numWorkItems = m_defaultWorkGroupSize*((numVertices + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		ciErrNum = clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_applyGravityKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);

		if (ciErrNum != CL_SUCCESS);
			return false;
	}

	//------------------
	// ApplyForcesKernel
	//------------------
	{
		cl_int ciErrNum;	
		ciErrNum = clSetKernelArg(m_applyForcesKernel, 0, sizeof(int), &numVertices);
		ciErrNum = clSetKernelArg(m_applyForcesKernel, 1, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_applyForcesKernel, 2, sizeof(cl_mem), &m_DBVertices);

		assert(ciErrNum == CL_SUCCESS);
		
		size_t m_defaultWorkGroupSize = 64;
		size_t numWorkItems = m_defaultWorkGroupSize*((numVertices + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		ciErrNum = clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_applyForcesKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);

		if (ciErrNum != CL_SUCCESS);
			return false;
	}

	//-----------
	// Integrate
	//-----------
	/*for ( int batch = 0; batch < (int)m_BatchSpringIndexArray.size()-1; batch++ )
	{
		int startSpringIndex = m_BatchSpringIndexArray[batch];
		int endSpringIndex = m_BatchSpringIndexArray[batch+1]-1;

		ciErrNum = clSetKernelArg(m_integrateKernel, 0, sizeof(int), &numSprings);
		ciErrNum = clSetKernelArg(m_integrateKernel, 1, sizeof(cl_mem), &startSpringIndex);
		ciErrNum = clSetKernelArg(m_integrateKernel, 2, sizeof(cl_mem), &endSpringIndex);
		ciErrNum = clSetKernelArg(m_integrateKernel, 3, sizeof(float), &dt);
		ciErrNum = clSetKernelArg(m_integrateKernel, 4, sizeof(cl_mem), &m_DBVertices);
		ciErrNum = clSetKernelArg(m_integrateKernel, 5, sizeof(cl_mem), &m_DBStrechSprings);

		assert(ciErrNum == CL_SUCCESS);
		
		numWorkItems = m_defaultWorkGroupSize*((numSprings + (m_defaultWorkGroupSize-1)) / m_defaultWorkGroupSize);

		ciErrNum = clEnqueueNDRangeKernel(g_cqGPUCommandQue, m_integrateKernel, 1, NULL, &numWorkItems, &m_defaultWorkGroupSize, 0,0,0);

		if (ciErrNum != CL_SUCCESS);
			return false;
	}*/

	//------------------------
	// Read data back to CPU
	//------------------------
	cl_int ciErrNum = clEnqueueReadBuffer(g_cqGPUCommandQue, m_DBVertices, CL_TRUE, 0, sizeof(VertexClothCL) * numVertices, m_HBVertexCL, 0, NULL, NULL);

	if (ciErrNum != CL_SUCCESS);
		return false;

	for ( int i = 0; i < numVertices; i++ )
	{
		const VertexClothCL& vertexData = m_HBVertexCL[i];

		/*m_VertexArray[i].m_Index = vertexData.m_Index;
		m_VertexArray[i].m_InvMass = vertexData.m_InvMass;
		m_VertexArray[i].m_PinIndex = vertexData.m_PinIndex;*/

		m_VertexArray[i].m_Pos.Set(vertexData.m_Pos.x, vertexData.m_Pos.y, vertexData.m_Pos.z);
	}

	return true;
}

bool CClothCL::AdvancePosition(btScalar dt)
{
	return CCloth::AdvancePosition(dt);
}

void CClothCL::ReleaseKernels()
{
	if ( !m_bBuildCLKernels )
		return;

	/*RELEASE_CL_KERNEL( m_prepareLinksKernel );
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
	RELEASE_CL_KERNEL( m_outputToVertexArrayKernel );*/
	RELEASE_CL_KERNEL( m_applyGravityKernel );
	RELEASE_CL_KERNEL( m_applyForcesKernel );
	RELEASE_CL_KERNEL( m_integrateKernel );
}

bool CClothCL::BuildCLKernels()
{
	if ( m_bBuildCLKernels )
		return true;

	ReleaseKernels();

	m_clFunctions.clearKernelCompilationFailures();

	m_applyGravityKernel = m_clFunctions.compileCLKernelFromString(ApplyForcesCLString, "ApplyGravityKernel");
	m_applyForcesKernel = m_clFunctions.compileCLKernelFromString(ApplyForcesCLString, "ApplyForcesKernel");
	m_integrateKernel = m_clFunctions.compileCLKernelFromString(ApplyForcesCLString, "Integrate");

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

bool ColoringCompare(const CSpringCloth3D& a, const CSpringCloth3D& b)
{
	return a.m_Coloring < b.m_Coloring;
}

void CClothCL::GenerateBatches()
{
	m_BatchSpringIndexArray.clear();

	for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
	{
		CSpringCloth3D& spring = m_StrechSpringArray[i];

		const CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		const CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		int coloring = 0;		

		while ( true ) 
		{
			bool bFound0 = false;
			bool bFound1 = false;

			for ( int a = 0; a < (int)vert0.m_StrechSpringIndexes.size(); a++ )
			{
				const CSpringCloth3D& otherSpring = m_StrechSpringArray[vert0.m_StrechSpringIndexes[a]];

				// skip if the neighbor spring is actually itself
				if ( otherSpring.GetIndex() == spring.GetIndex() )
					continue;

				if ( otherSpring.m_Coloring == coloring )
				{
					bFound0 = true;
					break;
				}				
			}
			
			for ( int a = 0; a < (int)vert1.m_StrechSpringIndexes.size(); a++ )
			{
				const CSpringCloth3D& otherSpring = m_StrechSpringArray[vert1.m_StrechSpringIndexes[a]];

				// skip if the neighbor spring is actually itself
				if ( otherSpring.GetIndex() == spring.GetIndex() )
					continue;

				if ( otherSpring.m_Coloring == coloring )
				{
					bFound1 = true;
					break;
				}				
			}

			if ( bFound0 || bFound1 )
				coloring++;
			else
				break;
		} 

		spring.m_Coloring = coloring;
	}

#ifdef _DEBUG

	for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
	{
		CSpringCloth3D& spring = m_StrechSpringArray[i];

		const CVertexCloth3D& vert0 = m_VertexArray[spring.GetVertexIndex(0)];
		const CVertexCloth3D& vert1 = m_VertexArray[spring.GetVertexIndex(1)];

		int coloring = spring.m_Coloring;
		bool bFound0 = false;
		bool bFound1 = false;

		for ( int a = 0; a < (int)vert0.m_StrechSpringIndexes.size(); a++ )
		{
			const CSpringCloth3D& otherSpring = m_StrechSpringArray[vert0.m_StrechSpringIndexes[a]];

			// skip if the neighbor spring is actually itself
			if ( otherSpring.GetIndex() == spring.GetIndex() )
				continue;

			if ( otherSpring.m_Coloring == coloring )
			{
				bFound0 = true;
				break;
			}				
		}
		
		for ( int a = 0; a < (int)vert1.m_StrechSpringIndexes.size(); a++ )
		{
			const CSpringCloth3D& otherSpring = m_StrechSpringArray[vert1.m_StrechSpringIndexes[a]];

			// skip if the neighbor spring is actually itself
			if ( otherSpring.GetIndex() == spring.GetIndex() )
				continue;

			if ( otherSpring.m_Coloring == coloring )
			{
				bFound1 = true;
				break;
			}				
		}

		assert(!bFound0 && !bFound1);
	}
#endif

	// Count how many batches were generated
	int countBatches = 0;

	for ( int i = 0; i < (int)m_StrechSpringArray.size(); i++ )
	{
		CSpringCloth3D& spring = m_StrechSpringArray[i];

		if ( spring.m_Coloring > countBatches )
			countBatches = spring.m_Coloring;
	}

	countBatches++;

	std::sort(m_StrechSpringArray.begin(), m_StrechSpringArray.end(), ColoringCompare);

	m_BatchSpringIndexArray.push_back(0);

	if ( m_StrechSpringArray.size() > 1 )
	{
		int i = 0;

		for ( i = 0; i < (int)m_StrechSpringArray.size()-1; i++ )
		{
			CSpringCloth3D& spring = m_StrechSpringArray[i];
			CSpringCloth3D& springNext = m_StrechSpringArray[i+1];

#ifdef _DEBUG
			assert(spring.m_Coloring <= springNext.m_Coloring);
#endif

			if ( spring.m_Coloring < springNext.m_Coloring )
				m_BatchSpringIndexArray.push_back(i+1);
		}

		m_BatchSpringIndexArray.push_back(i);
	}

#ifdef _DEBUG
	for ( int i = 0; i < (int)m_BatchSpringIndexArray.size()-1; i++ )
	{
		int startIndex = m_BatchSpringIndexArray[i];
		int endIndex = m_BatchSpringIndexArray[i+1] - 1;

		for ( int j = startIndex; j <= endIndex; j++ )
		{
			assert(m_StrechSpringArray[j].m_Coloring == m_StrechSpringArray[startIndex].m_Coloring);
		}
	}
#endif

}