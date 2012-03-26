#pragma once

#include "EPAAlgorithm.h"
#include "ConvexCollisionAlgorithm.h"

class CCollisionObject;
class CEPAEdge;
class CGJKSimplex;
class CWorldSimulation;

class CGJKAlgorithm : public IConvexCollisionAlgorithm
{
protected:
	CEPAAlgorithm m_EPAAlgorithm;

protected:
	// helper function to generate CollisionInfo
	bool GenerateCollisionInfo(const CCollisionObject& objA, const CCollisionObject& objB, const CTransform &transB2A, const CGJKSimplex& simplex, CVector3D v, btScalar distSqr, CNarrowCollisionInfo* pCollisionInfo) const;

public:
	CGJKAlgorithm();
	virtual ~CGJKAlgorithm(void);
	
	virtual bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
};

