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
	assert(objB.GetCollisionObjectType() == CCollisionObject::ConvexHull || objB.GetCollisionObjectType() == CCollisionObject::LineSegment ); 

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
		minkowskiSum.GetVertices() = objA.GetVertices();
		minkowskiSum.GetFaces() = objA.GetFaces();
		minkowskiSum.GetEdges() = objA.GetEdges();

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

		CVector3D vec = -(edgeVert1 - edgeVert0);

		// translate face plane if it is visible from the vector
		for ( int i = 0; i < (int)minkowskiSum.GetFaces().size(); i++ )
		{
			CTriangleFace& tri = minkowskiSum.GetFaces()[i];		
			CVector3D n = tri.GetNormal();
			tri.m_bFlag = false;

			float dot = n.Dot(vec);

			// the face is visible to 'vec' so it should be translated
			if ( dot > 0 )
			{
				tri.PlaneEquation()[3] -= dot;

				// flag meaning this face is translated.
				tri.m_bFlag = true;
			}
		}

		// Rembember the number of original faces from objA. Anyting beyond this will be a new face created by two edges from objA and objB.
		int numOriginalFaces = (int)minkowskiSum.GetFaces().size();

		// create a new face by cross product of two edges from objA and objB. 
		// The edge from objA is silhouette edges visible by the edge from objB
		for ( int i = 0; i < (int)minkowskiSum.GetEdges().size(); i++ )
		{
			CEdge& edge = minkowskiSum.GetEdges()[i];	

			CVector3D n0 = minkowskiSum.GetFaces()[edge.GetTriangleIndex(0)].GetNormal();
			CVector3D n1 = minkowskiSum.GetFaces()[edge.GetTriangleIndex(1)].GetNormal();

			float dot0 = n0.Dot(vec);
			float dot1 = n1.Dot(vec);
			bool bCreateNewFace = false;
			CVector3D edgeVecA;
			CVector3D vec0;

			if ( dot0 > 0 && dot1 <= 0	)
			{
				vec0 = minkowskiSum.GetVertices()[edge.GetVertexIndex(0)];
				CVector3D vec1 = minkowskiSum.GetVertices()[edge.GetVertexIndex(1)];

				edgeVecA =  vec1 - vec0;
				bCreateNewFace = true;
			}
			else if ( dot0 <= 0 && dot1 > 0	)
			{
				vec0 = minkowskiSum.GetVertices()[edge.GetVertexIndex(0)];
				CVector3D vec1 = minkowskiSum.GetVertices()[edge.GetVertexIndex(1)];

				edgeVecA =  vec0 - vec1;
				bCreateNewFace = true;
			}
			
			if ( bCreateNewFace )
			{
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
		float dist = minDist;

		int iterN = 0;

		for ( iterN = 0; iterN < (int)minkowskiSum.GetFaces().size(); iterN++ )
		{
			CTriangleFace& tri = minkowskiSum.GetFaces()[iterN];
		
			CVector3D pntA;
			dist =SignedDistanceFromPointToPlane(origin, tri.PlaneEquation(), &pntA);

			// If the distance is positive, the plane is a separating plane. 
			if ( dist > 0 )
			{
				pCollisionInfo->bIntersect |= false;
				break;
			}

			if ( dist > minDist )
			{
				minDist = dist;

				// original faces from objA
				if ( iterN < numOriginalFaces )
				{
					if ( tri.m_bFlag )
					{
						closestPointA = pntA + edgeVert0 - vec;						
					}
					else
					{
						closestPointA = pntA + edgeVert0;
					}

					closestPointB = objB.GetTransform().InverseOther() * transA2W * edgeVert0;
				}
				else // newly created faces by cross product of two edges from objA and objB
				{
					closestPointA = pntA + edgeVert0;
					closestPointB = objB.GetTransform().InverseOther() * transA2W * edgeVert0;
				}
			}
		}

		// the origin is inside. It means two objects are overlapping(colliding).
		if ( iterN == (int)minkowskiSum.GetFaces().size() )
		{
			// if the origin is inside, the minimum distance must be negative or zero. 
			assert(minDist <= 0);

			pCollisionInfo->bIntersect = true;
			pCollisionInfo->penetrationDepth = -minDist;

			pCollisionInfo->witnessPntA = closestPointA;
			pCollisionInfo->witnessPntB = closestPointB;
		}
	}
	
	return pCollisionInfo->bIntersect;
}




