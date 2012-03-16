#pragma once

#include <vector>
#include "CollisionObject.h"
#include "EPAEdge.h"
#include "Simplex.h"
#include "EPAAlgorithm.h"


class CClothSim3D;

//===========================
struct CNarrowCollisionInfo
//===========================
{
public:
	CNarrowCollisionInfo() : pObjA(NULL), pObjB(NULL), bIntersect(false), penetrationDepth(0) {}

	// Compiler provided copy constructor 'CNarrowCollisionInfo(CNarrowCollisionInfo& )' will be good enough.

	CNarrowCollisionInfo(CCollisionObject* pObjA, CCollisionObject* pObjB, bool bIntersect, const CVector3D& witnessPntA, 
						 const CVector3D& witnessPntB, double penetrationDepth) : pObjA(pObjA), pObjB(pObjB), bIntersect(bIntersect), 
																				  witnessPntA(witnessPntA), witnessPntB(witnessPntB), 
																				  penetrationDepth(penetrationDepth) {}

	CCollisionObject* pObjA;
	CCollisionObject* pObjB;
	bool bIntersect;
	CVector3D witnessPntA; // clostest point in object A in local space of object A
	CVector3D witnessPntB; // clostest point in object B in local space of object B
	double proximityDistance; 
	double penetrationDepth; // must be positive in case bIntersect is true.

	// Compiler provided assigne operator(=) will be good enough.
};

//====================
class CNarrowPhaseGJK
//====================
{
protected:
	CClothSim3D* m_pSimulation;
	CEPAAlgorithm m_EPAAlgorithm;

protected:
	bool RunEPAAlgorithmWithMargins(const CCollisionObject& objA, const CCollisionObject& objB, const CVector3D& v) const;
	
	// helper function to generate CollisionInfo
	bool GenerateCollisionInfo(const CCollisionObject& objA, const CCollisionObject& objB, const CTransform &transB2A, const CGJKSimplex& simplex, CVector3D v, double distSqr, CNarrowCollisionInfo* pCollisionInfo) const;

public:
	CNarrowPhaseGJK(CClothSim3D* pSimulation);
	virtual ~CNarrowPhaseGJK(void);

	std::vector<CCollisionObject*> m_CollisionObjectList;
	
	bool CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity = false);

};

