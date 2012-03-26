#include <cassert>
#include "BIMAlgorithm.h"
#include "GJKAlgorithm.h"
#include "CollisionObject.h"
#include "mathUtil.h"
#include "NarrowPhaseCollisionDetection.h"

CBIMAlgorithm::CBIMAlgorithm(void)
{
}

CBIMAlgorithm::~CBIMAlgorithm(void)
{
}

bool CBIMAlgorithm::CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/)
{
	assert(objB.GetCollisionObjectType() == CCollisionObject::Point);
	//assert(objA.GetCollisionObjectType() == CCollisionObject::ConvexHull);

	// Initialize collision info
	pCollisionInfo->bIntersect = false;
	pCollisionInfo->penetrationDepth = 0;
	pCollisionInfo->proximityDistance = 0;
	pCollisionInfo->pObjA = &objA;
	pCollisionInfo->pObjB = &objB;

	CTransform transW2A = objA.GetTransform().InverseOther();

	CVector3D point = transW2A * objB.GetTransform().GetTranslation();
	btScalar minDist = -SIMD_INFINITY;
	btScalar maxDist = SIMD_INFINITY;
	CVector3D closestPointA;

	for ( int i = 0; i < (int)objA.GetFaces().size(); i++ )
	{
		TriangleFace& tri = objA.GetFaces()[i];
		
		CVector3D pntA;
		btScalar dist =SignedDistanceFromPointToPlane(point, tri.planeEqn, &pntA);

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




