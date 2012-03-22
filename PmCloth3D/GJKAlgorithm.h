#pragma once

#include <vector>
#include "CollisionObject.h"
#include "EPAEdge.h"
#include "GJKSimplex.h"
#include "EPAAlgorithm.h"

class CWorldSimulation;

class CGJKAlgorithm
{
protected:
	CEPAAlgorithm m_EPAAlgorithm;

protected:
	// helper function to generate CollisionInfo
	bool GenerateCollisionInfo(const CCollisionObject& objA, const CCollisionObject& objB, const CTransform &transB2A, const CGJKSimplex& simplex, CVector3D v, double distSqr, CNarrowCollisionInfo* pCollisionInfo) const;

public:
	CGJKAlgorithm();
	virtual ~CGJKAlgorithm(void);
	
	bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
};

