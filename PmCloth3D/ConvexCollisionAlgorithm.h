#pragma once

class CCollisionObject;
class CNarrowCollisionInfo;

class IConvexCollisionAlgorithm
{
public:
	IConvexCollisionAlgorithm(void);
	virtual ~IConvexCollisionAlgorithm(void);

	virtual bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false) = 0;
};

