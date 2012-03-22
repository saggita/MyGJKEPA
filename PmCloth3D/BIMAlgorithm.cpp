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

// The normal vector of plane formed by p0, p1 and p2 is (p1-p0).Cross(p2-p0).Normalize().
// If point is on the positive side of plane, it returns positive distance. Otherwise, it returns negative distance. 
double SignedDistanceFromPointToPlane(const CVector3D& point, const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, CVector3D* closestPointInTriangle = NULL)
{
	CVector3D n = (p1-p0).Cross(p2-p0).Normalize();

	if ( n.LengthSqr() < 1e-6 )
		return 0;
	else
	{
		double dist = (point-p0).Dot(n);

		if ( closestPointInTriangle )
			*closestPointInTriangle = point - dist * n;

		return dist;
	}
}


bool CBIMAlgorithm::CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/)
{
	assert(objB.GetCollisionObjectType() == CCollisionObject::Point);
	assert(objA.GetCollisionObjectType() == CCollisionObject::ConvexHull);

	// Initialize collision info
	pCollisionInfo->bIntersect = false;
	pCollisionInfo->penetrationDepth = 0;
	pCollisionInfo->proximityDistance = 0;
	pCollisionInfo->pObjA = &objA;
	pCollisionInfo->pObjB = &objB;

	CTransform transW2A = objA.GetTransform().InverseOther();

	CVector3D point = transW2A * objB.GetTransform().GetTranslation();
	double minDist = -DBL_MAX;
	CVector3D closestPointA;

	for ( int i = 0; i < (int)objA.GetFaces().size(); i++ )
	{
		TriangleFace& tri = objA.GetFaces()[i];
		CVector3D* vert[3];

		for ( int j = 0; j < 3; j++ )
			vert[j] = &objA.GetVertices()[tri.indices[j]]; 
		
		CVector3D pntA;
		double dist =SignedDistanceFromPointToPlane(point, *vert[0], *vert[1], *vert[2], &pntA);

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




