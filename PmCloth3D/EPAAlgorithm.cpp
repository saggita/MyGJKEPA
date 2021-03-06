#define NOMINMAX

#include <cassert>
#include <algorithm>
#include "EPAAlgorithm.h"
#include "NarrowPhaseCollisionDetection.h"
#include "CollisionObject.h"
#include "GJKSimplex.h"
#include "EPAEdge.h"
#include "EPATriangle.h"

CEPAAlgorithm::CEPAAlgorithm(void)
{
}

CEPAAlgorithm::~CEPAAlgorithm(void)
{
}

bool CEPAAlgorithm::ComputePenetrationDepthAndContactPoints(const CGJKSimplex& simplex, CCollisionObject& objA, CCollisionObject& objB, CVector3D& v, CNarrowCollisionInfo* pCollisionInfo, int maxIteration /*= 30*/)
{
	std::vector<CVector3D> suppPointsA;       
    std::vector<CVector3D> suppPointsB;    
    std::vector<CVector3D> points;   

	suppPointsA.reserve(20);
	suppPointsB.reserve(20);
	points.reserve(20);

	// Initialize collision info
	pCollisionInfo->bIntersect = false;
	pCollisionInfo->penetrationDepth = 0;
	pCollisionInfo->proximityDistance = 0;
	pCollisionInfo->pObjA = &objA;
	pCollisionInfo->pObjB = &objB;

	// transform a local position in objB space to local position in objA space
	CTransform transB2A = objA.GetTransform().InverseOther() * objB.GetTransform();

	// transform a local position in objA space to local position in objB space
	CTransform transA2B = objB.GetTransform().InverseOther() * objA.GetTransform();

	// rotate matrix which transform a local vector in objA space to local vector in objB space
	CMatrix33 rotA2B = objB.GetTransform().GetRotation().GetMatrix33().TransposeOther() * objA.GetTransform().GetRotation().GetMatrix33();

	int numVertices = simplex.GetPoints(suppPointsA, suppPointsB, points);
	m_Polytope.Clear();

	assert(numVertices == points.size());
	assert(m_Polytope.GetTriangles().size() == 0);

	switch ( numVertices )
	{
		case 1:
			// Two objects are barelly touching. 
			return false;
		case 2:
			// The origin lies in a line segment. 
			// We create a hexahedron which is glued two tetrahedrons. It is explained in Geno's book. 


			break;
		case 3:
			{
				// The origin lies in a triangle. 
				// Add two new vertices to create a hexahedron. It is explained in Geno's book. 
				CVector3D n = (points[1] - points[0]).Cross(points[2] - points[0]);
				CVector3D w0 =  objA.GetLocalSupportPoint(n) - transB2A * objB.GetLocalSupportPoint(rotA2B * (-n));
				CVector3D w1 =  objA.GetLocalSupportPoint(-n) - transB2A * objB.GetLocalSupportPoint(rotA2B * n);

				if ( !m_Polytope.AddHexahedron(points[0], points[1], points[2], w0, w1) )
					return false;
			}
			break;
		case 4:
			{
				// In this cae, simplex computed from GJK is a tetrahedron. 
				if ( !m_Polytope.AddTetrahedron(points[0], points[1], points[2], points[3]) )
					return false;
			}
			break;
	}

	assert(m_Polytope.GetVertices().size() > 0);

	// Now we can expand the polytope which contains the origin to get the penetration depth and contact points. 
	float upperBound = FLT_MAX;
	float lowerBound = -FLT_MAX;
		
	int numIter = 0;

	while ( numIter < maxIteration )
	{
		CEPATriangle* pClosestTriangle = m_Polytope.PopAClosestTriangleToOriginFromHeap();

		assert(pClosestTriangle != NULL);

		v = pClosestTriangle->GetClosestPoint();

		CVector3D supportPointA = objA.GetLocalSupportPoint(v, objA.GetMargin());
		CVector3D supportPointB = transB2A * objB.GetLocalSupportPoint(rotA2B * (-v), objB.GetMargin());

		/*CVector3D supportPointA = objA.GetLocalSupportPoint(v);
		CVector3D supportPointB = transB2A * objB.GetLocalSupportPoint(rotA2B * (-v));*/

		CVector3D w = supportPointA - supportPointB;

		// Compute upper and lower bounds
		upperBound = min(upperBound, w * v.NormalizeOther());

#ifdef _DEBUG
 		//lowerBound = std::max(lowerBound, v.Length());
		//assert(lowerBound == v.Length());
#endif

		//lowerBound = v.Length();
		lowerBound = max(lowerBound, v.Length());

		if ( upperBound - lowerBound < 1e-4 || numIter == maxIteration - 1 )
		{
			pCollisionInfo->bIntersect = true;
			pCollisionInfo->penetrationDepth = 0.5f * (upperBound + lowerBound);
			pCollisionInfo->witnessPntA = pClosestTriangle->GetClosestPointToOriginInSupportPntSpace(suppPointsA);
			pCollisionInfo->witnessPntB = transA2B * pClosestTriangle->GetClosestPointToOriginInSupportPntSpace(suppPointsB);
			pCollisionInfo->proximityDistance = 0;
			pCollisionInfo->pObjA = &objA;
			pCollisionInfo->pObjB = &objB;

			break;
		}

		if ( !m_Polytope.ExpandPolytopeWithNewPoint(w, pClosestTriangle) )
		{
			pCollisionInfo->bIntersect = false;
			return false;
		}

		suppPointsA.push_back(supportPointA);
		suppPointsB.push_back(supportPointB);
		points.push_back(w);

		numIter++;
	}

	return pCollisionInfo->bIntersect;
}




