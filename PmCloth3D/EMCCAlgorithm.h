#pragma once

#include "ConvexCollisionAlgorithm.h"

class CCollisionObject;
class CNarrowCollisionInfo;

// Explicit Minkowski Convex Collision Algorithm
class CEMCCAlgorithm : public IConvexCollisionAlgorithm
{
public:
	CEMCCAlgorithm(void);
	virtual ~CEMCCAlgorithm(void);

	virtual bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
private:
	bool InternalCheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
};

