#pragma once

#include "ConvexCollisionAlgorithm.h"

class CCollisionObject;
class CNarrowCollisionInfo;

// Convex Height Field method.
class CCHFAlgorithm : public IConvexCollisionAlgorithm
{
public:
	CCHFAlgorithm(void);
	virtual ~CCHFAlgorithm(void);

	virtual bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
};

