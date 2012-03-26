#pragma once

#include "ConvexCollisionAlgorithm.h"

class CCollisionObject;
class CNarrowCollisionInfo;

// Brute-force iterative method to compute narrowphase collision info 
class CBIMAlgorithm : public IConvexCollisionAlgorithm
{
public:
	CBIMAlgorithm(void);
	virtual ~CBIMAlgorithm(void);

	virtual bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
};

