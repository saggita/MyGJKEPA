#pragma once

#include "ConvexCollisionAlgorithm.h"

class CCollisionObject;
class CNarrowCollisionInfo;

class CLIBCCDGJKAlgorithm : public IConvexCollisionAlgorithm
{
public:
	CLIBCCDGJKAlgorithm(void);
	virtual ~CLIBCCDGJKAlgorithm(void);

	virtual bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
};

	