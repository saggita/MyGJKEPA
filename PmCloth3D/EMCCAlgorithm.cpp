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
	assert(objA.GetCollisionObjectType() == CCollisionObject::ConvexHull);
	assert(objB.GetCollisionObjectType() == CCollisionObject::ConvexHull); 

	// Initialize collision info
	pCollisionInfo->bIntersect = false;
	pCollisionInfo->penetrationDepth = 0;
	pCollisionInfo->proximityDistance = 0;
	pCollisionInfo->pObjA = &objA;
	pCollisionInfo->pObjB = &objB;

	CVector3D closestPointA;
	CVector3D closestPointB;
	CVector3D origin(0, 0, 0);

	CTransform transA2W = objA.GetTransform();
	CTransform transW2A = objA.GetTransform().InverseOther();

	for ( int e = 0; e < (int)objB.GetEdges().size(); e++ )
	{
		CCollisionObject minkowskiSum;
		minkowskiSum.GetFaces() = objA.GetFaces();

		// edgeVert0 and edgeVert1 are in local frame of objA
		CVector3D edgeVert0 = transW2A * objB.GetTransform() * objB.GetVertices()[objB.GetEdges()[e].GetVertexIndex(0)];
		CVector3D edgeVert1 = transW2A * objB.GetTransform() * objB.GetVertices()[objB.GetEdges()[e].GetVertexIndex(1)];

		// translate all of faces so that edgeVert0 can be origin
		for ( int i = 0; i < (int)minkowskiSum.GetFaces().size(); i++ )
		{
			CTriangleFace& tri = minkowskiSum.GetFaces()[i];
			CVector3D n = tri.GetNormal();

			float d = n.Dot(-edgeVert0);

			tri.PlaneEquation()[3] -= d;
		}

		CVector3D vec = edgeVert1 - edgeVert0;

		// translate face plane if the edge is on the same side of the face normal
		for ( int i = 0; i < (int)minkowskiSum.GetFaces().size(); i++ )
		{
			CTriangleFace& tri = minkowskiSum.GetFaces()[i];		
			CVector3D n = tri.GetNormal();

			float dot = n.Dot(vec);

			if ( dot > 0 )
			{
				tri.PlaneEquation()[3] -= dot;
			}
		}

		int numOriginalFaces = (int)minkowskiSum.GetFaces().size();

		for ( int i = 0; i < (int)minkowskiSum.GetEdges().size(); i++ )
		{
			CEdge& edge = minkowskiSum.GetEdges()[i];	

			float dot0 = minkowskiSum.GetFaces()[edge.GetTriangleIndex(0)].GetNormal().Dot(vec);
			float dot1 = minkowskiSum.GetFaces()[edge.GetTriangleIndex(1)].GetNormal().Dot(vec);

			if ( dot0 > 0 && dot1 <= 0	)
			{
				CVector3D vec0 = minkowskiSum.GetVertices()[edge.GetVertexIndex(0)];
				CVector3D vec1 = minkowskiSum.GetVertices()[edge.GetVertexIndex(1)];

				CVector3D edgeVecA =  vec1 - vec0;

				CVector3D n = edgeVecA.Cross(vec).Normalize();
				float d = vec0.Dot(n);

				CTriangleFace newFace;
				newFace.PlaneEquation()[0] = n.m_X;
				newFace.PlaneEquation()[1] = n.m_Y;
				newFace.PlaneEquation()[2] = n.m_Z;
				newFace.PlaneEquation()[3] = -d;

				minkowskiSum.GetFaces().push_back(newFace);
			}
			else if ( dot0 <= 0 && dot1 > 0	)
			{
				CVector3D vec0 = minkowskiSum.GetVertices()[edge.GetVertexIndex(0)];
				CVector3D vec1 = minkowskiSum.GetVertices()[edge.GetVertexIndex(1)];

				CVector3D edgeVecA =  vec0 - vec1;

				CVector3D n = edgeVecA.Cross(vec).Normalize();
				float d = vec0.Dot(n);

				CTriangleFace newFace;
				newFace.PlaneEquation()[0] = n.m_X;
				newFace.PlaneEquation()[1] = n.m_Y;
				newFace.PlaneEquation()[2] = n.m_Z;
				newFace.PlaneEquation()[3] = -d;

				minkowskiSum.GetFaces().push_back(newFace);
			}
		}

		//--------------------------------------------------------------------------------------
		// Now, check if the origin is the inside of the newly constructed convex minkowski sum
		//--------------------------------------------------------------------------------------
		float minDist = -FLT_MAX;
		float maxDist = FLT_MAX;		

		int iterN = 0;

		for ( iterN = 0; iterN < (int)minkowskiSum.GetFaces().size(); iterN++ )
		{
			CTriangleFace& tri = minkowskiSum.GetFaces()[iterN];
		
			CVector3D pntA;
			float dist =SignedDistanceFromPointToPlane(origin, tri.PlaneEquation(), &pntA);

			// If the distance is positive, the plane is a separating plane. 
			if ( dist > 0 )
			{
				pCollisionInfo->bIntersect |= false;
				break;
			}

			if ( dist > minDist )
			{
				minDist = dist;
				closestPointA = pntA;
			}
		}

		// the origin is inside.
		if ( iterN == (int)minkowskiSum.GetFaces().size() )
		{
			pCollisionInfo->bIntersect = true;
		}
	}
	
	return pCollisionInfo->bIntersect;
}




