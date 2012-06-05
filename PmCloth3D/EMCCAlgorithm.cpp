#include <cassert>
#include "EMCCAlgorithm.h"
#include "GJKAlgorithm.h"
#include "CollisionObject.h"
#include "mathUtil.h"
#include "NarrowPhaseCollisionDetection.h"

CEMCCAlgorithm::CEMCCAlgorithm(void)
{
}

CEMCCAlgorithm::~CEMCCAlgorithm(void)
{
}

bool CEMCCAlgorithm::CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/)
{
	//assert(objB.GetCollisionObjectType() == CCollisionObject::Point); 
	//assert(objA.GetCollisionObjectType() == CCollisionObject::ConvexHull);

	// Initialize collision info
	pCollisionInfo->bIntersect = false;
	pCollisionInfo->penetrationDepth = 0;
	pCollisionInfo->proximityDistance = 0;
	pCollisionInfo->pObjA = &objA;
	pCollisionInfo->pObjB = &objB;

	CTransform transW2A = objA.GetTransform().InverseOther();

	CVector3D point = transW2A * objB.GetTransform().GetTranslation();
	float minDist = -FLT_MAX;
	float maxDist = FLT_MAX;
	CVector3D closestPointA;

	for ( int i = 0; i < (int)objA.GetFaces().size(); i++ )
	{
		CTriangleFace& tri = objA.GetFaces()[i];
		
		CVector3D pntA;
		float dist =SignedDistanceFromPointToPlane(point, tri.PlaneEquation(), &pntA);

		// If the distance is positive, the plane is a separating plane. 
		if ( dist > 0 )
			return false;

		if ( dist > minDist )
		{
			minDist = dist;
			closestPointA = pntA;
		}
	}

	pCollisionInfo->bIntersect = true;
	pCollisionInfo->penetrationDepth = -minDist;
	pCollisionInfo->witnessPntA = closestPointA;
	pCollisionInfo->witnessPntB = CVector3D(0, 0, 0);

	return pCollisionInfo->bIntersect;
}




