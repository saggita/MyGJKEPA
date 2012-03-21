#pragma once

#include <vector>

class CCollisionObject;
class CNarrowCollisionInfo;

// Brute-force interative method to compute narrowphase collision info 
class CBIMAlgorithm
{
public:
	CBIMAlgorithm(void);
	virtual ~CBIMAlgorithm(void);

	std::vector<CCollisionObject*> m_CollisionObjectList;
	bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);
};

