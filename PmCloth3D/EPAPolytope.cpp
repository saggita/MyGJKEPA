#include <algorithm>
#include <cassert>
#include "EPAPolytope.h"

CEPAPolytope::CEPAPolytope(void) : m_Count(0)
{
}

CEPAPolytope::~CEPAPolytope(void)
{
	Clear();
}

void CEPAPolytope::Clear()
{
	for ( unsigned int i = 0; i < m_Triangles.size(); i++ )
		delete m_Triangles[i];

	m_Count = 0;
	m_Triangles.clear();

	m_Vertices.clear();
	m_SilhouetteVertices.clear();
	m_SilhouetteTriangles.clear();
}

CEPATriangle* CEPAPolytope::PopAClosestTriangleToOriginFromHeap()
{
	CEPATriangle* pReturnTriangle = NULL;

	if ( m_Triangles.size() == 0 )
		return pReturnTriangle;

	CEPATriangleComparison compare;

	// TODO: Need to figure out how to utilize std::pop_heap

	//while ( 1 )
	//{
	//	pReturnTriangle = m_Triangles.front();
	//	std::pop_heap(m_Triangles.begin(), m_Triangles.end(), compare);

	//	if ( !pReturnTriangle->IsObsolete() && pReturnTriangle->IsClosestPointInternal() )
	//	{
	//		m_Triangles.pop_back();
	//		break;
	//	}

	//	if ( !pReturnTriangle->IsClosestPointInternal() )
	//	{

	//	}

	//	// If the popped triangle has external point close to the origin, this is a problem.
	//	// We already assigne max value to those triangles and they should not be popped here. 
	//	assert(pReturnTriangle->IsClosestPointInternal());

	//	if ( m_Triangles.size() == 0 )
	//	{
	//		pReturnTriangle = NULL;
	//		break;
	//	}
	//} 

	double minDistSqr = DBL_MAX;

	for ( int i = 0; i < (int)m_Triangles.size(); i++ )
	{
		if ( !m_Triangles[i]->IsObsolete() && m_Triangles[i]->IsClosestPointInternal() )
		{
			if ( m_Triangles[i]->GetDistSqr() < minDistSqr )
			{
				minDistSqr = m_Triangles[i]->GetDistSqr();
				pReturnTriangle = m_Triangles[i];
			}
		}
	}

	return pReturnTriangle;
}

#ifdef _DEBUG
static bool CheckWinding(const CVector3D& p0, const CVector3D& p1, const CVector3D& p2)
{
	return (p1-p0).Cross(p2-p0).Dot(p0) > 0;
}
#endif

bool CEPAPolytope::AddTetrahedron(const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, const CVector3D& p3)
{
	int index[4];
	m_Vertices.push_back(p0);
	index[0] = (int)m_Vertices.size() - 1;

	m_Vertices.push_back(p1);
	index[1] = (int)m_Vertices.size() - 1;

	m_Vertices.push_back(p2);
	index[2] = (int)m_Vertices.size() - 1;

	m_Vertices.push_back(p3);
	index[3] = (int)m_Vertices.size() - 1;

	CEPATriangle* pTri[4];

	if ( (p1-p0).Cross(p2-p0).Dot(p0) > 0 ) // p0, p1, p2 winding in counter-clockwise
	{
		assert((p1-p0).Cross(p2-p0).Dot(p3) < 0 ); // tet must contain the origin.

		pTri[0] = new CEPATriangle(index[0], index[1], index[2]); assert(CheckWinding(p0, p1, p2));
		pTri[1] = new CEPATriangle(index[0], index[3], index[1]); assert(CheckWinding(p0, p3, p1));
		pTri[2] = new CEPATriangle(index[0], index[2], index[3]); assert(CheckWinding(p0, p2, p3));
		pTri[3] = new CEPATriangle(index[1], index[3], index[2]); assert(CheckWinding(p1, p3, p2));
	}
	else // p0, p2, p1 winding in counter-clockwise
	{
		assert((p2-p0).Cross(p1-p0).Dot(p3) < 0 ); // tet must contain the origin.

		pTri[0] = new CEPATriangle(index[0], index[2], index[1]); assert(CheckWinding(p0, p2, p1));
		pTri[1] = new CEPATriangle(index[0], index[3], index[2]); assert(CheckWinding(p0, p3, p2));
		pTri[2] = new CEPATriangle(index[0], index[1], index[3]); assert(CheckWinding(p0, p1, p3));
		pTri[3] = new CEPATriangle(index[2], index[3], index[1]); assert(CheckWinding(p2, p3, p1));		
	}

	// construct adjacency
	pTri[0]->m_AdjacentTriangles[0] = pTri[1];
	pTri[0]->m_AdjacentTriangles[1] = pTri[3];
	pTri[0]->m_AdjacentTriangles[2] = pTri[2];

	pTri[1]->m_AdjacentTriangles[0] = pTri[2];
	pTri[1]->m_AdjacentTriangles[1] = pTri[3];
	pTri[1]->m_AdjacentTriangles[2] = pTri[0];

	pTri[2]->m_AdjacentTriangles[0] = pTri[0];
	pTri[2]->m_AdjacentTriangles[1] = pTri[3];
	pTri[2]->m_AdjacentTriangles[2] = pTri[1];

	pTri[3]->m_AdjacentTriangles[0] = pTri[1];
	pTri[3]->m_AdjacentTriangles[1] = pTri[2];
	pTri[3]->m_AdjacentTriangles[2] = pTri[0];

	pTri[0]->m_Edges[0]->m_pPairEdge = pTri[1]->m_Edges[2];
	pTri[0]->m_Edges[1]->m_pPairEdge = pTri[3]->m_Edges[2];
	pTri[0]->m_Edges[2]->m_pPairEdge = pTri[2]->m_Edges[0];

	pTri[1]->m_Edges[0]->m_pPairEdge = pTri[2]->m_Edges[2];
	pTri[1]->m_Edges[1]->m_pPairEdge = pTri[3]->m_Edges[0];
	pTri[1]->m_Edges[2]->m_pPairEdge = pTri[0]->m_Edges[0];

	pTri[2]->m_Edges[0]->m_pPairEdge = pTri[0]->m_Edges[2];
	pTri[2]->m_Edges[1]->m_pPairEdge = pTri[3]->m_Edges[1];
	pTri[2]->m_Edges[2]->m_pPairEdge = pTri[1]->m_Edges[0];

	pTri[3]->m_Edges[0]->m_pPairEdge = pTri[1]->m_Edges[1];
	pTri[3]->m_Edges[1]->m_pPairEdge = pTri[2]->m_Edges[1];
	pTri[3]->m_Edges[2]->m_pPairEdge = pTri[0]->m_Edges[1];

	CEPATriangleComparison compare;

	for ( int i = 0; i < 4; i++ )
	{
		pTri[i]->ComputeClosestPointToOrigin(*this);
		pTri[i]->m_Index = m_Count++;
		m_Triangles.push_back(pTri[i]);
		std::push_heap(m_Triangles.begin(), m_Triangles.end(), compare);
	}

	return true;
}

bool CEPAPolytope::AddPoint(const CVector3D& w, CEPATriangle* pTriangleUsedToObtainW)
{
	// First, we need to get a loop of edges using 'Flood Fill Silhouette' algorithm.
	for ( unsigned int i = 0; i < m_Triangles.size(); i++ )
	{
		m_Triangles[i]->SetVisited(false);
		m_Triangles[i]->m_bVisible = false;
	}

	pTriangleUsedToObtainW->SetVisited(false);

	m_SilhouetteVertices.clear();
	m_SilhouetteVertices.reserve(20);
	m_SilhouetteTriangles.clear();
	m_SilhouetteTriangles.reserve(20);
	m_VisibleTriangles.clear();
	m_SilhouetteEdges.clear();
	m_SilhouetteEdges.reserve(20);

	m_Vertices.push_back(w);
	int indexVertexW = (int)m_Vertices.size() - 1;

	assert(pTriangleUsedToObtainW->IsObsolete() == false);
	pTriangleUsedToObtainW->m_bVisible = true;

	for ( int i = 0; i < 3; i++ )
		pTriangleUsedToObtainW->m_Edges[i]->m_pPairEdge->m_pEPATriangle->DoSilhouette(w, pTriangleUsedToObtainW->m_Edges[i], *this);

	assert(m_SilhouetteVertices.size() >= 3);
	assert(m_SilhouetteTriangles.size() >= 3);

	// Now, we create new triangles to patch the silhouette loop 
	unsigned int silhouetteSize = m_SilhouetteVertices.size();

#ifdef _DEBUG
	for ( int i = 0; i < m_Triangles.size(); i++ )
	{
		if ( m_Triangles[i]->m_bVisible )
			assert(m_Triangles[i]->IsVisibleFromPoint(w) == true);
		else
			assert(m_Triangles[i]->IsVisibleFromPoint(w) == false);
	}
	
	for ( unsigned int i = 0; i < silhouetteSize; i++ )
	{
		assert(m_SilhouetteTriangles[i]->IsVisibleFromPoint(w) == false);
	}
#endif

	std::vector<CEPATriangle*> newTriangles;
	newTriangles.reserve(silhouetteSize);

	CEPATriangleComparison compare;

	for ( unsigned int i = 0; i < silhouetteSize; i++ )
	{
		int j = i+1 < silhouetteSize ? i+1 : 0;

		CEPATriangle* pTri = new CEPATriangle(indexVertexW, m_SilhouetteVertices[i], m_SilhouetteVertices[j]);
		newTriangles.push_back(pTri);
		pTri->ComputeClosestPointToOrigin(*this);
	}

	for ( int i = 0; i < silhouetteSize; i++ )
	{
		int j = (i+1 < silhouetteSize)? i+1 : 0;
		int k = (i-1 < 0)? silhouetteSize-1 : i-1;

		newTriangles[i]->m_AdjacentTriangles[2] = newTriangles[j];
		newTriangles[i]->m_Edges[2]->m_pPairEdge = newTriangles[j]->m_Edges[0];

		newTriangles[i]->m_AdjacentTriangles[0] = newTriangles[k];
		newTriangles[i]->m_Edges[0]->m_pPairEdge = newTriangles[j]->m_Edges[2];

		int indexEdge;
		int indexNextVertex = m_SilhouetteTriangles[i]->GetNextVertexIndexCCW(m_SilhouetteVertices[j], &indexEdge);

		assert(indexNextVertex == m_SilhouetteVertices[i]);
		assert(0 <= indexEdge && indexEdge < 3);
		
		m_SilhouetteTriangles[i]->m_AdjacentTriangles[indexEdge] = newTriangles[i];
		newTriangles[i]->m_AdjacentTriangles[1] = m_SilhouetteTriangles[i];
		newTriangles[i]->m_Edges[1]->m_pPairEdge = m_SilhouetteEdges[i];
		m_SilhouetteEdges[i]->m_pPairEdge = newTriangles[i]->m_Edges[1]->m_pPairEdge;
	}

	for ( unsigned int i = 0; i < silhouetteSize; i++ )
	{	
		newTriangles[i]->m_Index = m_Count++;
		m_Triangles.push_back(newTriangles[i]);
		std::push_heap(m_Triangles.begin(), m_Triangles.end(), compare);
	}

#ifdef _DEBUG
	for ( unsigned int i = 0; i < GetTriangles().size(); i++ )
	{
		if ( !GetTriangles()[i]->IsObsolete() )
		{
			for ( int j = 0; j < 3; j++ )
			{
				assert(GetTriangles()[i]->GetEdge(j)->GetIndexVertex(0) == GetTriangles()[i]->GetEdge(j)->m_pPairEdge->GetIndexVertex(1));
				assert(GetTriangles()[i]->GetEdge(j)->GetIndexVertex(1) == GetTriangles()[i]->GetEdge(j)->m_pPairEdge->GetIndexVertex(0));
			}
		}
	}
#endif

	return true;
}

//bool CEPAPolytope::DoSilhouette(const CVector3D& w, const CEPATriangle* pTriangle)
//{
//}