#include "NarrowPhaseGJK.h"
#include "EPAAlgorithm.h"
#include "Simplex.h"

CNarrowPhaseGJK::CNarrowPhaseGJK(CClothSim3D* pSimulation) : m_pSimulation(pSimulation)
{
	
}

CNarrowPhaseGJK::~CNarrowPhaseGJK(void)
{
}

bool CNarrowPhaseGJK::CalcCollisionInfo(const CCollisionObject& objA, const CCollisionObject& objB, const CTransform &transB2A, const CGJKSimplex& simplex, CVector3D v, double distSqr, CNarrowCollisionInfo* pCollisionInfo) const
{
	CVector3D closestPntA;
	CVector3D closestPntB;
	simplex.ClosestPointAandB(closestPntA, closestPntB);

	double dist = sqrt(distSqr);
	pCollisionInfo->proximityDistance = dist;

	assert(dist > 0.0);

	CVector3D n = v.NormalizeOther();

	closestPntA = closestPntA + (objA.GetMargin() * (-n));
	closestPntB = closestPntB + (objB.GetMargin() * (n));
	closestPntB = transB2A.InverseOther() * closestPntB;

	// normal vector of collision
	CVector3D normalCollision = objA.GetTransform().GetRotation() * (-n);
			
	// penetration depth
	double margin = objA.GetMargin() + objB.GetMargin();
	double penetrationDepth = margin - dist;

	// TODO: const is a problem..
	/*pCollisionInfo->pObjA = &objA;
	pCollisionInfo->pObjB = &objB;*/

	pCollisionInfo->penetrationDepth = penetrationDepth;
	pCollisionInfo->witnessPntA = closestPntA;
	pCollisionInfo->witnessPntB = closestPntB;

	if ( penetrationDepth <= 0 )
		pCollisionInfo->bIntersect = false;
	else
		pCollisionInfo->bIntersect = true;

	return pCollisionInfo->bIntersect;
}


bool CNarrowPhaseGJK::CheckCollision(const CCollisionObject& objA, const CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/) 
{
	CVector3D suppPntA; // support point from object A
	CVector3D suppPntB; // support point from object B
	CVector3D closestPntA;
	CVector3D closestPntB;
	CVector3D w; // support point of Minkowski difference(A-B)
	double vw; // v dot w
	double margin = objA.GetMargin() + objB.GetMargin();
	double marginSqr = margin * margin;
	CGJKSimplex simplex;

	// transform a local position in objB space to local position in objA space
	CTransform transB2A = objA.GetTransform().InverseOther() * objB.GetTransform();

	// rotate matrix which transform a local vector in objA space to local vector in objB space
	CQuaternion rotA2B = objB.GetTransform().GetRotation().InverseOther() * objA.GetTransform().GetRotation();

	// closest point to the origin
	// TODO:Need to use cached one to exploit frame coherence. 
	CVector3D v(1.0, 0.0, 0.0);

	double distSqr = DBL_MAX;
	double distSqrPrev = distSqr;

	int numIter = 0;
	
	while ( 1 ) 
	{
		// support points are in local space of objA
		suppPntA = objA.GetLocalSupportPoint(-v);
		suppPntB = transB2A * objB.GetLocalSupportPoint(rotA2B * v); 

		// w is also in local space of objA
		w = suppPntA - suppPntB;

		vw = v.Dot(w);
				
		// If v.Dot(w) > 0, it means there is a separating axis and objA and objB do not intersect.
		// If v.Dot(w)^2 / dist^2 > (margin from objA + margin from objB)^2, it means enlarged objects with margins do not intersect.
		// So we just exit this function if we don't check proximity. If we compute proximity(distance), we should not terminate here
		// and keep iterating until w becomes a part of simplex or dist is close enough to be proxmity distance satisfing (dist^2 - v*w <= dist^2 * tolerance). 
		// In this case, penetration depth will be negative and this function will return false since objects don't intersect. 
		if ( !bProximity && vw > 0.0 && vw*vw > (distSqr * marginSqr) )
			return false;

		// Check if w is a part of simplex already. If so, it means v == w and v is a closest point to origin in Minkowski differnce and 
		// objects are disjoint. 
		// If dist^2 - v*w <= dist^2 * tolerance, v is close enough to the closest point to origin in Minkowski differnce and 
		// ||v|| is a proximity distance. In this case, objects are disjoint because v * w > 0 and it means
		// v is a separting axis. 
		// 
		// TODO: Techinically, the scond condidion(dist^2 - v*w <= dist^2 * tolerance) will cover the first condition(w is a part of simplex already).
		//       A litle bit redundancy. 
		bool isDegenerate = simplex.IsDegenerate(w);
		if ( simplex.IsDegenerate(w) || distSqr - vw <= distSqr * 1e-6 )
		{
			return CalcCollisionInfo(objA, objB, transB2A, simplex, v, distSqr, pCollisionInfo);			
		}

		// Add w into simplex. Determinants will be computed inside AddPoint(..). 
		simplex.AddPoint(w, suppPntA, suppPntB);

		if ( !simplex.IsAffinelyIndependent() ) 
		{
			return CalcCollisionInfo(objA, objB, transB2A, simplex, v, distSqr, pCollisionInfo);
		}

		// Run Johnson's Algorithm
		// Using Johnson's Algorithm, we can calculate a new 'v' as well as the subset simplex which contains 'v'.
		// The subset simplex can be a vertex, edge, triangle or tetrahedron. 
		if ( !simplex.RunJohnsonAlgorithm(v) )
		{
			return CalcCollisionInfo(objA, objB, transB2A, simplex, v, distSqr, pCollisionInfo);			
		}

		distSqrPrev = distSqr;
		distSqr = v.LengthSqr();

		// If there is not much improvement since previous iteration
		// TODO: Do I need to check this? 
		/*if ( distSqrPrev - distSqr <= DBL_EPSILON * distSqrPrev )
		{
			return true;
		}*/

		// If simplex is full, we found a simplex(tetrahedron) containing origin. 
		// We stop iterating and pass this simplex to EPA to find penetration depth and closest points.
		if ( simplex.IsFull() )
			break;

		// Instead seeing if distSqr is zero within tolerance, we use relative tolerance using max sqaured lengh from simplex.
		// This is explained in 'Collision Detection in Interactive 3D'.
		// So if distSqr is less than or equal to EPSILON * (max squared length from simplex), we consider that we've found
		// a simplex containing origin. In this case, the simplex could be a vertex, a line segment, a triangle or a tetrahedron. 
		// We stop iterating and pass this simplex to EPA to find penetration depth and closest points.
		// TODO: FreeSolid is using 'approxZero(v)'. Do I need to use it rather than a line below?
		if ( distSqr <= DBL_EPSILON * simplex.MaxLengthSqr() ) 
			break;
				
		++numIter;
	} 

	//return RunEPAAlgorithmWithMargins(objA, objB, v);
	CEPAAlgorithm EPAAlgorithm;
	return EPAAlgorithm.ComputePenetrationDepthAndContactPoints(simplex, objA, objB, v);
}


bool CNarrowPhaseGJK::RunEPAAlgorithmWithMargins(const CCollisionObject& objA, const CCollisionObject& objB, const CVector3D& v) const
{
	//CVector3D suppPntA; // support point from object A
	//CVector3D suppPntB; // support point from object B
	//CVector3D w; // support point of Minkowski difference(A-B)
	//double vw; // v dot w
	//double margin = objA.GetMargin() + objB.GetMargin();
	//double marginSqr = margin * margin;
	//CGJKSimplex simplex;

	//// transform a local position in objB space to local position in objA space
	//CTransform transB2A = objA.GetTransform().InverseOther() * objB.GetTransform();

	//// rotate matrix which transform a local vector in objA space to local vector in objB space
	//CMatrix33 rotA2B = objB.GetTransform().GetRotation().GetMatrix33().TransposeOther() * objA.GetTransform().GetRotation().GetMatrix33();

	//double distSqr = DBL_MAX;
	//double distSqrPrev = distSqr;

	//int numIter = 0;

	return true;	
}


