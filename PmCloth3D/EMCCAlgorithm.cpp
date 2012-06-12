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

	CNarrowCollisionInfo BA;
	InternalCheckCollision(objB, objA, &BA, bProximity);

	if ( AB.bIntersect && BA.bIntersect )
	{
		if ( AB.penetrationDepth < BA.penetrationDepth )
			*pCollisionInfo = AB;
		else
			*pCollisionInfo = BA;

		return true;
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
		pCollisionInfo->pObjA = &objA;
		pCollisionInfo->pObjB = &objB;
		return false;
	}
}

bool CEMCCAlgorithm::InternalCheckCollision(CCollisionObject& objA, CCollisionObject& objB, CNarrowCollisionInfo* pCollisionInfo, bool bProximity/* = false*/)
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

	float maxDist = -FLT_MAX;

	for ( int e = 0; e < (int)objB.GetEdges().size(); e++ )
	{
		// We are trying to find out non-intersecting case here. So we start off with an assumption of positive case.  
		bool bIntersect = true;

		float minDist = -FLT_MAX;

		// edgeVert0 and edgeVert1 are in local frame of objA
		CVector3D edgeVert0 = transW2A * objB.GetTransform() * objB.GetVertices()[objB.GetEdges()[e].GetVertexIndex(0)];
		CVector3D edgeVert1 = transW2A * objB.GetTransform() * objB.GetVertices()[objB.GetEdges()[e].GetVertexIndex(1)];

		CVector3D n0B = transW2A * objB.GetTransform() * objB.GetFaces()[objB.GetEdges()[e].GetTriangleIndex(0)].GetNormal();
		CVector3D n1B = transW2A * objB.GetTransform() * objB.GetFaces()[objB.GetEdges()[e].GetTriangleIndex(1)].GetNormal();

		CVector3D vec = -(edgeVert1 - edgeVert0);

		for ( int i = 0; i < (int)objA.GetEdges().size(); i++ )
		{
			const CEdge& edge = objA.GetEdges()[i];

			CVector3D n0 = objA.GetFaces()[edge.GetTriangleIndex(0)].GetNormal();
			CVector3D n1 = objA.GetFaces()[edge.GetTriangleIndex(1)].GetNormal();

			float dot0 = n0.Dot(vec);
			float dot1 = n1.Dot(vec);

			// Find silhouette edges from objA which is visible by 'vec'.
			if ( (dot0 > 0 && dot1 <= 0) || (dot0 <= 0 && dot1 > 0) )
			{
				// translate vertices so that edgeVert0 can be origin
				CVector3D vec0 = objA.GetVertices()[edge.GetVertexIndex(0)] - edgeVert0;
				CVector3D vec1 = objA.GetVertices()[edge.GetVertexIndex(1)] - edgeVert0;
			
				CVector3D n = (vec1 - vec0).Cross(vec).Normalize();

				if ( n.Dot(n0) < 0 || n.Dot(n1) < 0 )
					n = -n;

				assert(n.Dot(n0) >= 0 && n.Dot(n1) >= 0);
				
				float dist = -vec1.Dot(n);
				CVector3D pntA;
				pntA = -dist * n;

				// If the distance is positive, we just found a separating axis
				if ( dist > 0 )
				{
					bIntersect = false;
					break;
				}
				else if ( dist > minDist )
				{
					// If 'n' is a separating axis
					if ( n.Dot(n0B) >= 0 && n.Dot(n1B) >= 0 )
					{
						minDist = dist;

						float t;
						CVector3D vecN;
						float distToEdge = DistancePointToEdge(pntA, vec0, vec1, t, vecN);

						assert( 0 <= t && t <= 1.0f );
					
						closestPointA = pntA - distToEdge*vecN + edgeVert0;
						closestPointB = -distToEdge*vecN + edgeVert0;
					}
				}
			}
		}

		if ( bIntersect )
		{
			for ( int iterN = 0; iterN < (int)objA.GetFaces().size(); iterN++ )
			{
				CTriangleFace tri = objA.GetFaces()[iterN];

				// translate the face so that edgeVert0 can be origin
				CVector3D n = tri.GetNormal();
				float d = n.Dot(-edgeVert0);
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

				// 'SignedDistanceFromPointToPlane' returns negative distance if the point is the other side of plane when cosidering plane normal vector. 
				float dist = SignedDistanceFromPointToPlane(origin, tri.PlaneEquation(), &pntA);

				// If the distance is positive, the plane is a separating plane. 
				if ( dist > 0 )
				{
					bIntersect = false;
					break;
				}
				else if ( dist > minDist )
				{
					if ( bTranslated )
					{
						const CVertex& vertB = objB.GetVertices()[objB.GetEdges()[e].GetVertexIndex(1)];

						bool bSeparatingPlane = true;
						const std::vector<int>& indexFaces = vertB.GetFaceIndeces();

						// check whether 'tri' is a separating plane. 
						for ( int m = 0; m < indexFaces.size(); m++ )
						{
							const CVector3D& normal = transW2A * objB.GetTransform() * objB.GetFaces()[indexFaces[m]].GetNormal();

							if ( n.Dot(normal) > 0 )
							{
								bSeparatingPlane = false;
								break;
							}
						}

						if ( bSeparatingPlane )
						{
							minDist = dist;
							closestPointA = pntA + edgeVert0 - vec;						
							closestPointB = edgeVert0 - vec;
						}
					}
					else
					{
						const CVertex& vertB = objB.GetVertices()[objB.GetEdges()[e].GetVertexIndex(0)];

						bool bSeparatingPlane = true;
						const std::vector<int>& indexFaces = vertB.GetFaceIndeces();

						// check whether 'tri' is a separating plane. 
						for ( int m = 0; m < indexFaces.size(); m++ )
						{
							const CVector3D& normal = transW2A * objB.GetTransform() * objB.GetFaces()[indexFaces[m]].GetNormal();

							if ( n.Dot(normal) > 0 )
							{
								bSeparatingPlane = false;
								break;
							}
						}

						if ( bSeparatingPlane )
						{
							minDist = dist;
							closestPointA = pntA + edgeVert0;
							closestPointB = edgeVert0;
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

			if ( -minDist > maxDist )
			{
				pCollisionInfo->penetrationDepth = -minDist;
				pCollisionInfo->witnessPntA = closestPointA;
				pCollisionInfo->witnessPntB = objB.GetTransform().InverseOther() * transA2W * closestPointB;

				maxDist = -minDist;
			}
		}
	}
	
	return pCollisionInfo->bIntersect;
}




