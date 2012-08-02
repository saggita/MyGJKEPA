#pragma once

#include <CL\cl.h>
#include "CLFunctions.h"
#include "ConvexCollisionAlgorithm.h"
#include "CollisionObjectCL.h"

class CCollisionObject;
class CNarrowCollisionInfo;

// Explicit Minkowski Convex Collision Algorithm in OpenCL
class CEMCCAlgorithmCL : public IConvexCollisionAlgorithm
{
public:
	CEMCCAlgorithmCL(void);
	virtual ~CEMCCAlgorithmCL(void);

	virtual bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
private:
	bool InternalCheckCollision(CCollisionObjectCL* pObjA, CCollisionObjectCL* pObjB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);

	cl_mem m_DBObjA;
	cl_mem m_DBObjB;

	cl_kernel m_EMCCKernel;
	CLFunctions m_clFunctions;
};

