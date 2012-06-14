#define NOMINMAX

#include <algorithm>
#include <limits>
#include <cassert>
#include "EMCCAlgorithm.h"
#include "GJKAlgorithm.h"
#include "CollisionObject.h"
#include "mathUtil.h"
#include "NarrowPhaseCollisionDetection.h"
#include "CollisionDetections.h"

CEMCCAlgorithm::CEMCCAlgorithm(void)
{
}

CEMCCAlgorithm::~CEMCCAlgorithm(void)
{
}

bool CEMCCAlgorithm::CheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/)
{
	CNarrowCollisionInfo AB;
	InternalCheckCollision(objA, objB, &AB, bProximity);

	/*CNarrowCollisionInfo BA;
	InternalCheckCollision(objB, objA, &BA, bProximity);

	if ( AB.bIntersect && BA.bIntersect )
	{
		if ( AB.penetrationDepth < BA.penetrationDepth )
			*pCollisionInfo = AB;
		else
			*pCollisionInfo = BA;
	}
	else if ( AB.bIntersect && !BA.bIntersect )
	{
		*pCollisionInfo = AB;
	}
	else if ( !AB.bIntersect && BA.bIntersect )
	{
		*pCollisionInfo = BA;
	}
	else
	{
		pCollisionInfo->bIntersect = false;
		pCollisionInfo->penetrationDepth = 0;
		pCollisionInfo->proximityDistance = 0;		
	}*/

	*pCollisionInfo = AB;

	pCollisionInfo->pObjA = &objA;
	pCollisionInfo->pObjB = &objB;

	return pCollisionInfo->bIntersect;
}

bool CEMCCAlgorithm::InternalCheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/)
{
	assert(objA.GetCollisionObjectType() == CCollisionObject::ConvexHull || objA.GetCollisionObjectType() == CCollisionObject::LineSegment);
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

	float maxDist = -FLT_MAX;
	

	for ( int e = 0; e < (int)objB.GetEdges().size(); e++ )
	{
		float minDist = -FLT_MAX;

		// We are trying to find out non-intersecting case here. So we start off with an assumption of positive case.  
		bool bIntersect = true;

		const CEdge& edgeB = objB.GetEdges()[e];

		// edgeVert0B and edgeVert1B are from objB and defined in local frame of objA
		CVector3D edgeVert0B = transW2A * objB.GetTransform() * objB.GetVertices()[edgeB.GetVertexIndex(0)];
		CVector3D edgeVert1B = transW2A * objB.GetTransform() * objB.GetVertices()[edgeB.GetVertexIndex(1)];

		CVector3D n0B = transW2A.GetRotation() * objB.GetTransform().GetRotation() * objB.GetFaces()[edgeB.GetTriangleIndex(0)].GetNormal();
		CVector3D n1B = transW2A.GetRotation() * objB.GetTransform().GetRotation() * objB.GetFaces()[edgeB.GetTriangleIndex(1)].GetNormal();

		CVector3D vec = -(edgeVert1B - edgeVert0B);

		for ( int i = 0; i < (int)objA.GetEdges().size(); i++ )
		{
			CEdge& edgeA = objA.GetEdges()[i];
			edgeA.m_bFlag = false;

			CVector3D n0 = objA.GetFaces()[edgeA.GetTriangleIndex(0)].GetNormal();
			CVector3D n1 = objA.GetFaces()[edgeA.GetTriangleIndex(1)].GetNormal();

			float dot0 = n0.Dot(vec);
			float dot1 = n1.Dot(vec);

			// Find silhouette edges from objA which is visible by 'vec'.
			if ( (dot0 > 0 && dot1 <= 0) || (dot0 <= 0 && dot1 > 0) )
			{
				edgeA.m_bFlag = true;

				// translate vertices so that edgeVert0B can be origin
				CVector3D vec0A = objA.GetVertices()[edgeA.GetVertexIndex(0)] - edgeVert0B;
				CVector3D vec1A = objA.GetVertices()[edgeA.GetVertexIndex(1)] - edgeVert0B;
			
				CVector3D n = (vec1A - vec0A).Cross(vec).Normalize();

				////////////////////////////////////////////////////
				// WRONG!!!!
				if ( n.Dot(n0) < 0 || n.Dot(n1) < 0 )
					n = -n;

				assert(n.Dot(n0) >= 0 && n.Dot(n1) >= 0);
				////////////////////////////////////////////////////

				//-------------------------------------------
				// Check whether 'n' may be a seprating axis
				//-------------------------------------------
				bool bMayBeSeparatingAxis = true;

				for ( int twoEnds = 0; twoEnds < 2; twoEnds++ )
				{
					const std::vector<int>& indexEdges = objB.GetVertices()[edgeB.GetVertexIndex(twoEnds)].GetEdgeIndeces();

					for ( int a = 0; a < (int)indexEdges.size(); a++ )
					{
						if ( edgeB.GetIndex() == indexEdges[a] )
							continue;

						int idx = objB.GetEdges()[indexEdges[a]].GetVertexIndex(0);

						if ( idx == edgeB.GetVertexIndex(0) )
							idx = objB.GetEdges()[indexEdges[a]].GetVertexIndex(1);

						CVector3D vecE = objB.GetVertices()[idx] - objB.GetVertices()[edgeB.GetVertexIndex(twoEnds)];
						vecE = transW2A.GetRotation() * objB.GetTransform().GetRotation() * vecE;

						if ( n.Dot(vecE) < 0 )
						{
							bMayBeSeparatingAxis = false;
							break;
						}
					}

					if ( !bMayBeSeparatingAxis )
						break;
				}

				if ( bMayBeSeparatingAxis )
				{
					// distance from origin to the face created by the cross product of 'edgeA' and 'edgeB' in the local frame of 'objA'.
					float dist = -vec1A.Dot(n);
					CVector3D pntA;
					pntA = -dist * n;
				
					// If 'n' is a separating axis
					if ( dist > 0 )
					{
						bIntersect = false;
					}
					else if ( dist > minDist )
					{
						minDist = dist;

						/*float t;
						CVector3D vecN;
						float distToEdge = DistancePointToEdge(pntA, vec0A, vec1A, t, vecN);

						assert( 0 <= t && t <= 1.0f );

						vecN = -vec.NormalizeOther();
					
						closestPointB = -distToEdge*vecN + edgeVert0B;
						closestPointA = pntA + closestPointB;					*/

						float p, q;
						CVector3D vecN;
						float distEdgeEdge = DistanceEdgeToEdge(vec0A + edgeVert0B, vec1A + edgeVert0B, edgeVert0B, edgeVert1B, p, q, vecN, false);

						closestPointA = p*(vec0A + edgeVert0B) + (1.0f-p)*(vec1A + edgeVert0B);
						closestPointB = q*edgeVert0B + (1.0f-q)*edgeVert1B;
					}
				}
			}
		}

		if ( bIntersect )
		{
			for ( int iterN = 0; iterN < (int)objA.GetFaces().size(); iterN++ )
			{
				CTriangleFace tri = objA.GetFaces()[iterN];

				// translate the face so that edgeVert0B can be origin
				CVector3D n = tri.GetNormal();
				float d = n.Dot(-edgeVert0B);
				tri.PlaneEquation()[3] -= d;

				// translate face plane if it is visible from the vector
				bool bTranslated = false;
				float dot = n.Dot(vec);

				// the face is visible to 'vec' so it should be translated
				if ( dot > 0 )
				{
					tri.PlaneEquation()[3] -= dot;
					bTranslated = true;
				}
		
				CVector3D pntA;

				// 'SignedDistanceFromPointToPlane' returns negative distance if the point is in the other side of plane when cosidering plane normal vector. 
				float dist = SignedDistanceFromPointToPlane(origin, tri.PlaneEquation(), &pntA);

				// Check whether 'tri' may be a separating plane or not.
				bool bMayBeSeparatingPlane = true;

				if ( bTranslated )
				{
					const CVertex& vertB = objB.GetVertices()[edgeB.GetVertexIndex(1)];
					const std::vector<int>& indexEdges = vertB.GetEdgeIndeces();

					for ( int m = 0; m < (int)indexEdges.size(); m++ )
					{
						int idx = objB.GetEdges()[indexEdges[m]].GetVertexIndex(0);

						if ( idx == edgeB.GetVertexIndex(1) )
							idx = objB.GetEdges()[indexEdges[m]].GetVertexIndex(1);

						CVector3D vecE = objB.GetVertices()[idx] - objB.GetVertices()[edgeB.GetVertexIndex(1)];
						vecE = transW2A.GetRotation() * objB.GetTransform().GetRotation() * vecE;

						if ( n.Dot(vecE) < 0 )
						{
							bMayBeSeparatingPlane = false;
							break;
						}
					}
				}
				else
				{
					const CVertex& vertB = objB.GetVertices()[edgeB.GetVertexIndex(0)];
					const std::vector<int>& indexEdges = vertB.GetEdgeIndeces();

					for ( int m = 0; m < (int)indexEdges.size(); m++ )
					{
						int idx = objB.GetEdges()[indexEdges[m]].GetVertexIndex(0);

						if ( idx == edgeB.GetVertexIndex(0) )
							idx = objB.GetEdges()[indexEdges[m]].GetVertexIndex(1);

						CVector3D vecE = objB.GetVertices()[idx] - objB.GetVertices()[edgeB.GetVertexIndex(1)];
						vecE = transW2A.GetRotation() * objB.GetTransform().GetRotation() * vecE;

						if ( n.Dot(vecE) < 0 )
						{
							bMayBeSeparatingPlane = false;
							break;
						}
					}
				}

				if ( bMayBeSeparatingPlane )
				{
					// If the distance is positive, the plane is a separating plane. 
					if ( dist > 0 )
					{
						bIntersect = false;
					}
					else if ( dist > minDist )
					{
					
						if ( bTranslated )
						{
							minDist = dist;
							closestPointA = pntA + edgeVert0B - vec;						
							closestPointB = edgeVert0B - vec;
						}
						else
						{
							minDist = dist;
							closestPointA = pntA + edgeVert0B;
							closestPointB = edgeVert0B;
						}					
					}
				}
			}
		}		

		if ( bIntersect )
		{
			// if the origin is inside, the minimum distance must be negative or zero. 
			assert(minDist <= 0);

			pCollisionInfo->bIntersect = true;
			
			if ( minDist > maxDist )
			{
				pCollisionInfo->penetrationDepth = -minDist;
				pCollisionInfo->witnessPntA = closestPointA;
				pCollisionInfo->witnessPntB = objB.GetTransform().InverseOther() * transA2W * closestPointB;

				maxDist = minDist;
			}
		}
	}
	
	/*if ( pCollisionInfo->bIntersect )
	{
		assert(maxDist <= 0);

		pCollisionInfo->penetrationDepth = -maxDist;
		pCollisionInfo->witnessPntA = closestPointA;
		pCollisionInfo->witnessPntB = objB.GetTransform().InverseOther() * transA2W * closestPointB;
	}*/

	return pCollisionInfo->bIntersect;
}




