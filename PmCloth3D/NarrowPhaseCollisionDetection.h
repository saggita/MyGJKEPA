#pragma once

#include <vector>
#include "Vector3D.h"
#include "ICollidable.h"

class CCollisionObject;
class CGJKAlgorithm;
class CBIMAlgorithm;
class CCHFAlgorithm;
class IConvexCollisionAlgorithm;

class CNarrowCollisionInfo
{
public:
	CNarrowCollisionInfo() : pObjA(NULL), pObjB(NULL), bIntersect(false), penetrationDepth(0) {}

	// Compiler provided copy constructor 'CNarrowCollisionInfo(CNarrowCollisionInfo& )' will be good enough.

	CNarrowCollisionInfo(ICollidable* pObjA, ICollidable* pObjB, bool bIntersect, const CVector3D& witnessPntA, 
		const CVector3D& witnessPntB, btScalar penetrationDepth) : pObjA(pObjA->GetCollisionObject()), pObjB(pObjB->GetCollisionObject()), bIntersect(bIntersect), 
																				  witnessPntA(witnessPntA), witnessPntB(witnessPntB), 
																				  penetrationDepth(penetrationDepth) {}

	CNarrowCollisionInfo(ICollidable* pObjA, ICollidable* pObjB) : pObjA(pObjA->GetCollisionObject()), pObjB(pObjB->GetCollisionObject()), bIntersect(false), 																				  
																				  penetrationDepth(0), proximityDistance(0) {}

	CCollisionObject* pObjA;
	CCollisionObject* pObjB;
	bool bIntersect;
	CVector3D witnessPntA; // clostest point in object A in local space of object A
	CVector3D witnessPntB; // clostest point in object B in local space of object B
	btScalar proximityDistance; 
	btScalar penetrationDepth; // must be positive in case bIntersect is true.

	// Compiler provided assign operator(=) will be good enough.
};

class CNarrowPhaseCollisionDetection
{
public:
	enum CollisionAlgorithmType { GJK_EPA, BIM, CHF };

	CNarrowPhaseCollisionDetection(void);
	virtual ~CNarrowPhaseCollisionDetection(void);

protected:
	CollisionAlgorithmType m_AlgorithmType;
	IConvexCollisionAlgorithm* m_pAlgorithm;
	std::vector<CNarrowCollisionInfo> m_CollisionPairs;

public:
	std::vector<CNarrowCollisionInfo>& GetPairs() { return m_CollisionPairs; }
	const std::vector<CNarrowCollisionInfo>& GetPairs() const { return m_CollisionPairs; }
	void AddPair(const CNarrowCollisionInfo pair) { m_CollisionPairs.push_back(pair); }
	void SetConvexCollisionAlgorithmType(CollisionAlgorithmType algorithmType);
	CollisionAlgorithmType GetConvexCollisionAlgorithmType() const { return m_AlgorithmType; }
	IConvexCollisionAlgorithm* GetConvexCollisionAlgorithm() { return m_pAlgorithm; }
	const IConvexCollisionAlgorithm* GetConvexCollisionAlgorithm() const { return m_pAlgorithm; }

	int CheckCollisions();
};

