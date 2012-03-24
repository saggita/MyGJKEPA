#pragma once

#include <vector>

class CCollisionObject;
class CNarrowCollisionInfo;

class CCHFAlgorithm
{
public:
	CCHFAlgorithm(void);
	virtual ~CCHFAlgorithm(void);

	std::vector<CCollisionObject*> m_CollisionObjectList;
	bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
};

