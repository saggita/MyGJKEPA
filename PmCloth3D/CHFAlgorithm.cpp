#include "CHFAlgorithm.h"
#include <cassert>
#include "GJKAlgorithm.h"
#include "CollisionObject.h"
#include "mathUtil.h"
#include "NarrowPhaseCollisionDetection.h"

CCHFAlgorithm::CCHFAlgorithm(void)
{
}

CCHFAlgorithm::~CCHFAlgorithm(void)
{
}

bool CCHFAlgorithm::CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/)
{
	if ( objA.GetCollisionObjectType() != CCollisionObject::ConvexHull || objB.GetCollisionObjectType() != CCollisionObject::Point )
		return false;	

	// Initialize collision info
	pCollisionInfo->bIntersect = false;
	pCollisionInfo->penetrationDepth = 0;
	pCollisionInfo->proximityDistance = 0;
	pCollisionInfo->pObjA = &objA;
	pCollisionInfo->pObjB = &objB;

	CTransform transW2A = objA.GetTransform().InverseOther();

	CVector3D point = transW2A * objB.GetTransform().GetTranslation();
	CVector3D closestPointA;

	assert(objA.GetConvexHFObject() != NULL);

	float4 normal;
	bool bNormal = objA.GetConvexHFObject()->queryDistanceWithNormal(make_float4(point.m_X, point.m_Y, point.m_Z), normal);
	float dist = objA.GetConvexHFObject()->queryDistance(make_float4(point.m_X, point.m_Y, point.m_Z));

	if ( dist >= 0 )
		return false;

	closestPointA =  (point + CVector3D(normal.x, normal.y, normal.z) * (-dist));

	pCollisionInfo->bIntersect = true;
	pCollisionInfo->penetrationDepth = -dist;
	pCollisionInfo->witnessPntA = closestPointA;
	pCollisionInfo->witnessPntB = CVector3D(0, 0, 0);

	return pCollisionInfo->bIntersect;
}
