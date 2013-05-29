#define NOMINMAX

#include <algorithm>
#include <limits>
#include <cassert>
#include "LIBCCDGJKAlgorithm.h"
#include "CollisionObject.h"
#include "mathUtil.h"
#include "NarrowPhaseCollisionDetection.h"
#include "CollisionDetections.h"
#include <ccd\ccd.h>
#include <ccd\supportfn.h>

CLIBCCDGJKAlgorithm::CLIBCCDGJKAlgorithm(void)
{
}

CLIBCCDGJKAlgorithm::~CLIBCCDGJKAlgorithm(void)
{
}

bool CLIBCCDGJKAlgorithm::CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/)
{
	if ( objA.GetCollisionObjectType() != CCollisionObject::Box || objB.GetCollisionObjectType() != CCollisionObject::Box )
		return false;

	CNarrowCollisionInfo colInfo;

	colInfo.bIntersect = false;
	colInfo.pObjA = &objA;
	colInfo.pObjB = &objB;
	*pCollisionInfo = colInfo;

	// libccd
	ccd_t ccd;
	CCD_INIT(&ccd);

	ccd.support1 = ccdSupport;
	ccd.support2 = ccdSupport;
	ccd.center1 = ccdObjCenter;
	ccd.center2 = ccdObjCenter;
	ccd.max_iterations = 100;
	ccd.mpr_tolerance = 1e-12;


	ccd_box_t boxA;
	boxA.type = CCD_OBJ_BOX;
	ccdVec3Set(&boxA.pos, objA.GetTransform().GetTranslation().m_X, objA.GetTransform().GetTranslation().m_Y, objA.GetTransform().GetTranslation().m_Z);
	ccdQuatSet(&boxA.quat, objA.GetTransform().GetRotation().m_X, objA.GetTransform().GetRotation().m_Y, objA.GetTransform().GetRotation().m_Z, objA.GetTransform().GetRotation().m_W);
	boxA.x = objA.GetSize().m_X;
	boxA.y = objA.GetSize().m_Y;
	boxA.z = objA.GetSize().m_Z;

	ccd_box_t boxB;
	boxB.type = CCD_OBJ_BOX;
	ccdVec3Set(&boxB.pos, objB.GetTransform().GetTranslation().m_X, objB.GetTransform().GetTranslation().m_Y, objB.GetTransform().GetTranslation().m_Z);
	ccdQuatSet(&boxB.quat, objB.GetTransform().GetRotation().m_X, objB.GetTransform().GetRotation().m_Y, objB.GetTransform().GetRotation().m_Z, objB.GetTransform().GetRotation().m_W);
	boxB.x = objB.GetSize().m_X;
	boxB.y = objB.GetSize().m_Y;
	boxB.z = objB.GetSize().m_Z;

	ccd_real_t depth;
	ccd_vec3_t dir, pos;
	
	//int res = ccdGJKPenetration(&boxA, &boxB, &ccd, &depth, &dir, &pos);
	int res = ccdMPRPenetration(&boxA, &boxB, &ccd, &depth, &dir, &pos);

	if ( res >= 0 )
	{
		colInfo.bIntersect = true;

		CVector3D posW(pos.v[0], pos.v[1], pos.v[2]);
		colInfo.witnessPntA = objA.GetTransform().InverseOther() * posW;
		colInfo.witnessPntB = objB.GetTransform().InverseOther() * posW;
	}

	colInfo.penetrationDepth = depth;
	*pCollisionInfo = colInfo;

	return colInfo.bIntersect;
}



