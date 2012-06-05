#pragma once

#include "ConvexCollisionAlgorithm.h"

class CCollisionObject;
class CNarrowCollisionInfo;

// Brute-force iterative method to compute narrowphase collision info 
class CEMCCAlgorithm : public IConvexCollisionAlgorithm
{
public:
	CEMCCAlgorithm(void);
	virtual ~CEMCCAlgorithm(void);

	virtual bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
};

