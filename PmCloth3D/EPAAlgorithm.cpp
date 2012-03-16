#define NOMINMAX

#include <algorithm>
#include "EPAAlgorithm.h"
#include "NarrowPhaseGJK.h"

//===================
// Global functions
//===================
bool link(const CEPAEdge& edge0, const CEPAEdge& edge1)
{
	bool bOK = (edge0.GetSourceVertexIndex() == edge1.GetTargetVertexIndex() &&
				edge0.GetTargetVertexIndex() == edge1.GetSourceVertexIndex());

	if ( bOK )
	{
		edge0.GetEPATriangle()->m_AdjacentEdges[edge0.GetIndexLocal()] = edge1;
		edge1.GetEPATriangle()->m_AdjacentEdges[edge1.GetIndexLocal()] = edge0;
	}

	return bOK;
}

void halfLink(const CEPAEdge& edge0, const CEPAEdge& edge1)
{
	assert(edge0.GetSourceVertexIndex() == edge1.GetTargetVertexIndex() &&
		   edge0.GetTargetVertexIndex() == edge1.GetSourceVertexIndex());

	edge0.GetEPATriangle()->m_AdjacentEdges[edge0.GetIndexLocal()] = edge1;
}

//=====================
// CEPAAlgorithm
//=====================

CEPAAlgorithm::CEPAAlgorithm(void)
{
}


CEPAAlgorithm::~CEPAAlgorithm(void)
{
}

void CEPAAlgorithm::AddFaceCandidate(CEPATriangle* triangle, CEPATriangle** heap, unsigned int& nbTriangles, double upperBoundSquarePenDepth)
{
	if ( triangle->IsClosestPointInternal() && triangle->GetDistSqr() <= upperBoundSquarePenDepth )
	{
		heap[nbTriangles] = triangle;
		++nbTriangles;
		std::push_heap(&heap[0], &heap[nbTriangles], triangleComparison);
	}
}

// Decide if the origin is in the tetrahedron
// Return 0 if the origin is in the tetrahedron and return the number (1,2,3 or 4) of
// the vertex that is wrong if the origin is not in the tetrahedron
int CEPAAlgorithm::IsOriginInTetrahedron(const CVector3D& p1, const CVector3D& p2, const CVector3D& p3, const CVector3D& p4) const
{
	// Check vertex 1
	CVector3D normal1 = (p2-p1).Cross(p3-p1);
    if (normal1.Dot(p1) > 0.0 == normal1.Dot(p4) > 0.0) {
        return 4;
    }

    // Check vertex 2
    CVector3D normal2 = (p4-p2).Cross(p3-p2);
    if (normal2.Dot(p2) > 0.0 == normal2.Dot(p1) > 0.0) {
        return 1;
    }

    // Check vertex 3
    CVector3D normal3 = (p4-p3).Cross(p1-p3);
    if (normal3.Dot(p3) > 0.0 == normal3.Dot(p2) > 0.0) {
        return 2;
    }

    // Check vertex 4
    CVector3D normal4 = (p2-p4).Cross(p1-p4);
    if (normal4.Dot(p4) > 0.0 == normal4.Dot(p3) > 0.0) {
        return 3;
    }

    // The origin is in the tetrahedron, we return 0
    return 0;
}

bool CEPAAlgorithm::ComputePenetrationDepthAndContactPoints(const CGJKSimplex& simplex, const CCollisionObject& objA, const CCollisionObject& objB, CVector3D& v)
{
	CVector3D suppPointsA[MAX_SUPPORT_POINTS];       // Support points of object A in local coordinates
    CVector3D suppPointsB[MAX_SUPPORT_POINTS];       // Support points of object B in local coordinates
    CVector3D points[MAX_SUPPORT_POINTS];            // Current points
    CTrianglesStore triangleStore;                   // Store the triangles
    CEPATriangle* triangleHeap[MAX_FACETS];          // Heap that contains the face candidate of the EPA algorithm

	// transform a local position in objB space to local position in objA space
	CTransform transB2A = objA.GetTransform().InverseOther() * objB.GetTransform();

	// rotate matrix which transform a local vector in objA space to local vector in objB space
	CMatrix33 rotA2B = objB.GetTransform().GetRotation().GetMatrix33().TransposeOther() * objA.GetTransform().GetRotation().GetMatrix33();

	unsigned int numVertices = simplex.getSimplex(suppPointsA, suppPointsB, points);
	unsigned int numTriangles = 0;

	triangleStore.clear();
	m_Polytope.Clear();

	switch ( numVertices )
	{
		case 1:
			// Two objects are barelly touching. 
			//CNarrowCollisionInfo collisionInfo(&objA, &objB, transB2A, simplex, CVector3D(0, 0, 0), 0);
			//return collisionInfo.bIntersect;
			return false;
		case 2:
			// The origin lies in a line segment. 
			// We create a hexahedron which is glued two tetrahedrons based on Geno's book. 


			break;

		case 3:
			// The origin lies in a triangle. 
			// Add two new vertices to create a hexahedron based on Geno's book. 


			break;
		case 4:
			{
				// In this cae, simplex computed from GJK is a tetrahedron. We need to check if this tetrahedron contains the origin. 
				int a = IsOriginInTetrahedron(points[0], points[1], points[2], points[3]);

				if ( a == 0 )
				{	

					m_Polytope.AddTetrahedron(points[0], points[1], points[2], points[3]);
				}
				else 
				{

					// TODO..

				}
			}
			break;
	}

	/*if ( numTriangles == 0 )
		return false;*/

	// Now we can expand the polytope which contains the origin to get the penetration depth and contact points. 
	CEPATriangle* triangle = 0;
    double upperBound = DBL_MAX;
	double lowerBound = -DBL_MAX;

	int numIter = 0;



	while ( 1 )
	{

#ifdef _DEBUG
		for ( unsigned int i = 0; i < m_Polytope.GetTriangles().size(); i++ )
		{
			if ( !m_Polytope.GetTriangles()[i]->IsObsolete() )
			{
				assert(m_Polytope.GetTriangles()[i]->GetEdge(i)->GetIndexVertex(0) == m_Polytope.GetTriangles()[i]->GetEdge(i)->m_pPairEdge->GetIndexVertex(1));
				assert(m_Polytope.GetTriangles()[i]->GetEdge(i)->GetIndexVertex(1) == m_Polytope.GetTriangles()[i]->GetEdge(i)->m_pPairEdge->GetIndexVertex(0));
			}
		}
#endif

		CEPATriangle* pClosestTriangle = m_Polytope.PopAClosestTriangleToOriginFromHeap();

		if ( !pClosestTriangle )
		{
			assert(0); // Should not reach here.
			return false;
		}

		v = pClosestTriangle->GetClosestPoint();

		suppPointsA[numVertices] = objA.GetLocalSupportPoint(v, objA.GetMargin());
		suppPointsB[numVertices] = transB2A * objB.GetLocalSupportPoint(rotA2B * (-v), objB.GetMargin());
		CVector3D w = suppPointsA[numVertices] - suppPointsB[numVertices];

		// Compute upper bound and lower bound
		upperBound = std::min(upperBound, w * v.NormalizeOther());
		lowerBound = std::max(lowerBound, v.Length());

		// lowerBound should increase monotonically
		assert(lowerBound == v.Length());

		if ( upperBound - lowerBound < 1e-6 )
			break;

		/*if ( v.Dot(w) - pClosestTriangle->GetDistSqr() < 1e-12 )
			break;*/

		m_Polytope.AddPoint(w, pClosestTriangle);
		numIter++;
	}

	return true;
}




