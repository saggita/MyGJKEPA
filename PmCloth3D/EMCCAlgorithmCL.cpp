#define NOMINMAX

#include <algorithm>
#include <limits>
#include <cassert>
#include "EMCCAlgorithmCL.h"
#include "GJKAlgorithm.h"
#include "CollisionObject.h"
#include "mathUtil.h"
#include "NarrowPhaseCollisionDetection.h"
#include "CollisionDetections.h"

#define MSTRINGIFY(A) #A

static const char* EMCCKernelString = 
#include "OpenCLC10/EMCC.cl"

extern cl_context        g_cxGPUMainContext;
extern cl_command_queue  g_cqGPUCommandQue;

CEMCCAlgorithmCL::CEMCCAlgorithmCL(void)
{
	m_clFunctions.m_cxMainContext = g_cxGPUMainContext;
	m_clFunctions.m_cqCommandQue = g_cqGPUCommandQue;

	m_clFunctions.clearKernelCompilationFailures();
	m_EMCCKernel = m_clFunctions.compileCLKernelFromString(EMCCKernelString, "EMCCKernel");

	assert( m_clFunctions.getKernelCompilationFailures() == 0 );

	m_DBObjA = clCreateBuffer(g_cxGPUMainContext, CL_MEM_READ_WRITE, sizeof(CCollisionObjectCL), NULL, NULL);
	m_DBObjB = clCreateBuffer(g_cxGPUMainContext, CL_MEM_READ_WRITE, sizeof(CCollisionObjectCL), NULL, NULL);

}

CEMCCAlgorithmCL::~CEMCCAlgorithmCL(void)
{
	if ( m_DBObjA )
		clReleaseMemObject(m_DBObjA);

	if ( m_DBObjB )
		clReleaseMemObject(m_DBObjB);

	if ( m_EMCCKernel )
	{
		clReleaseKernel(m_EMCCKernel);
		m_EMCCKernel = NULL;
	}
}

bool CEMCCAlgorithmCL::CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/)
{
	CCollisionObjectCL* pCObjCL_A = new CCollisionObjectCL();
	CCollisionObjectCL* pCObjCL_B = new CCollisionObjectCL();

	MakeCCollisionObjectCL(objA, pCObjCL_A);
	MakeCCollisionObjectCL(objB, pCObjCL_B);

	CNarrowCollisionInfo AB;
	InternalCheckCollision(pCObjCL_A, pCObjCL_B, &AB, bProximity);

	CNarrowCollisionInfo BA;

	if ( AB.bIntersect )
		InternalCheckCollision(pCObjCL_B, pCObjCL_A, &BA, bProximity);

	if ( AB.bIntersect && BA.bIntersect )
	{
		if ( AB.penetrationDepth < BA.penetrationDepth )
			*pCollisionInfo = AB;
		else
		{
			pCollisionInfo->bIntersect = BA.bIntersect;
			pCollisionInfo->penetrationDepth = BA.penetrationDepth;
			pCollisionInfo->proximityDistance = BA.proximityDistance;
			pCollisionInfo->pObjA = BA.pObjB;
			pCollisionInfo->pObjB = BA.pObjA;
			pCollisionInfo->witnessPntA = BA.witnessPntB;
			pCollisionInfo->witnessPntB = BA.witnessPntA;
		}
	}	
	else
	{
		pCollisionInfo->bIntersect = false;
		pCollisionInfo->penetrationDepth = 0;
		pCollisionInfo->proximityDistance = 0;		
	}

	delete pCObjCL_A;
	delete pCObjCL_B;

	return pCollisionInfo->bIntersect;
}

bool CEMCCAlgorithmCL::InternalCheckCollision(CCollisionObjectCL* pObjA, CCollisionObjectCL* pObjB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/)
{
	// Update OpenCL buffers
	cl_int result = clEnqueueWriteBuffer(g_cqGPUCommandQue, m_DBObjA, CL_TRUE, 0, sizeof(CCollisionObjectCL), pObjA, 0, NULL, NULL);
	assert(result == CL_SUCCESS);	

	result = clEnqueueWriteBuffer(g_cqGPUCommandQue, m_DBObjB, CL_TRUE, 0, sizeof(CCollisionObjectCL), pObjB, 0, NULL, NULL);
	assert(result == CL_SUCCESS);	

	// Execute OpenCL kernel


	return false;
}




