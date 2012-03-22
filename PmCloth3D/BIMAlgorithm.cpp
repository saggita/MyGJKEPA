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
	assert(objA.GetCollisionObjectType() == CCollisionObject::ConvexHull);

	// Initialize collision info
	pCollisionInfo->bIntersect = false;
	pCollisionInfo->penetrationDepth = 0;
	pCollisionInfo->proximityDistance = 0;
	pCollisionInfo->pObjA = &objA;
	pCollisionInfo->pObjB = &objB;

	CTransform transW2A = objA.GetTransform().InverseOther();

	CVector3D point = transW2A * objB.GetTransform().GetTranslation();
	double minDist = DBL_MAX;
	CVector3D closestPointA;

	for ( int i = 0; i < (int)objA.GetFaces().size(); i++ )
	{
		TriangleFace& tri = objA.GetFaces()[i];
		CVector3D* vert[3];

		for ( int j = 0; j < 3; j++ )
			vert[j] = &objA.GetVertices()[tri.indices[j]]; 

		// First check if objB(point) is on the positive side of triangle. If so, the triangle is separating plane and two objects don't intersect at all.
		//if ( (point - *vert[0]).Dot(objA.GetNormals()[i]) >= 0 )

		double dot = (point - *vert[0]).Dot((*vert[1]-*vert[0]).Cross(*vert[2]-*vert[0]));
		if ( dot >= 0 )
			return false;

		CVector3D pntA;
		double dist = DistanceFromPointToTriangle(point, *vert[0], *vert[1], *vert[2], &pntA);

		if ( dist < minDist )
		{
			minDist = dist;
			closestPointA = pntA;
		}
	}

	pCollisionInfo->bIntersect = true;
	pCollisionInfo->penetrationDepth = minDist;
	pCollisionInfo->witnessPntA = closestPointA;
	pCollisionInfo->witnessPntB = CVector3D(0, 0, 0);

	return pCollisionInfo->bIntersect;
}




