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
	m_SilhouetteEdges.clear();
	m_SupportPointsA.clear();
	m_SupportPointsB.clear();
}

CEPATriangle* CEPAPolytope::PopAClosestTriangleToOriginFromHeap()
{
	CEPATriangle* pReturnTriangle = NULL;

	if ( m_Triangles.size() == 0 )
		return pReturnTriangle;

	// TODO: Need to figure out how to utilize std::pop_heap
	
	// CEPATriangleComparison compare;

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

	float minDistSqr = FLT_MAX;

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
		//assert((p1-p0).Cross(p2-p0).Dot(p3) < 0 ); // tet must contain the origin.

		// tet must contain the origin.
		if ( (p1-p0).Cross(p2-p0).Dot(p3) >= 0 )
			return false;

		pTri[0] = new CEPATriangle(index[0], index[1], index[2]); assert(CheckWinding(p0, p1, p2));
		pTri[1] = new CEPATriangle(index[0], index[3], index[1]); assert(CheckWinding(p0, p3, p1));
		pTri[2] = new CEPATriangle(index[0], index[2], index[3]); assert(CheckWinding(p0, p2, p3));
		pTri[3] = new CEPATriangle(index[1], index[3], index[2]); assert(CheckWinding(p1, p3, p2));
	}
	else // p0, p2, p1 winding in counter-clockwise
	{
		//assert((p2-p0).Cross(p1-p0).Dot(p3) < 0 ); // tet must contain the origin.

		// tet must contain the origin.
		if ( (p2-p0).Cross(p1-p0).Dot(p3) >= 0 )
			return false;

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

#ifdef _DEBUG
		pTri[i]->m_Index = m_Count++;
#endif

		m_Triangles.push_back(pTri[i]);
		std::push_heap(m_Triangles.begin(), m_Triangles.end(), compare);
	}

	return true;
}

// p0, p1 and p2 form a triangle. With this triangle, p and q create two tetrahedrons. 
// w0 is in the side of the direction which is calculated by (p1-p0).Cross(p2-p0)
// w1 is in the side of the direction which is calculated by -(p1-p0).Cross(p2-p0)
// By gluing these two tetrahedrons, hexahedron can be formed.
bool CEPAPolytope::AddHexahedron(const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, const CVector3D& w0, const CVector3D& w1)
{
	/*assert((p1-p0).Cross(p2-p0).Dot(w0-p0) > 0);
	assert((p1-p0).Cross(p2-p0).Dot(w1-p0) < 0);*/

	if ( (p1-p0).Cross(p2-p0).Dot(w0-p0) <= 0 )
		return false;

	if ( (p1-p0).Cross(p2-p0).Dot(w1-p0) >= 0 )
		return false;

	int index[5];
	m_Vertices.push_back(p0);
	index[0] = (int)m_Vertices.size() - 1;

	m_Vertices.push_back(p1);
	index[1] = (int)m_Vertices.size() - 1;

	m_Vertices.push_back(p2);
	index[2] = (int)m_Vertices.size() - 1;

	m_Vertices.push_back(w0);
	index[3] = (int)m_Vertices.size() - 1;

	m_Vertices.push_back(w1);
	index[4] = (int)m_Vertices.size() - 1;

	CEPATriangle* pTri[6];

	pTri[0] = new CEPATriangle(index[0], index[1], index[3]); assert(CheckWinding(m_Vertices[index[0]], m_Vertices[index[1]], m_Vertices[index[3]]));
	pTri[1] = new CEPATriangle(index[1], index[2], index[3]); assert(CheckWinding(m_Vertices[index[1]], m_Vertices[index[2]], m_Vertices[index[3]]));
	pTri[2] = new CEPATriangle(index[2], index[0], index[3]); assert(CheckWinding(m_Vertices[index[2]], m_Vertices[index[0]], m_Vertices[index[3]]));
	pTri[3] = new CEPATriangle(index[1], index[0], index[4]); assert(CheckWinding(m_Vertices[index[1]], m_Vertices[index[0]], m_Vertices[index[4]]));
	pTri[4] = new CEPATriangle(index[2], index[1], index[4]); assert(CheckWinding(m_Vertices[index[2]], m_Vertices[index[1]], m_Vertices[index[4]]));
	pTri[5] = new CEPATriangle(index[0], index[2], index[4]); assert(CheckWinding(m_Vertices[index[0]], m_Vertices[index[2]], m_Vertices[index[4]]));
	
	// construct adjacency
	pTri[0]->m_AdjacentTriangles[0] = pTri[3];
	pTri[0]->m_AdjacentTriangles[1] = pTri[1];
	pTri[0]->m_AdjacentTriangles[2] = pTri[2];

	pTri[1]->m_AdjacentTriangles[0] = pTri[4];
	pTri[1]->m_AdjacentTriangles[1] = pTri[2];
	pTri[1]->m_AdjacentTriangles[2] = pTri[0];

	pTri[2]->m_AdjacentTriangles[0] = pTri[5];
	pTri[2]->m_AdjacentTriangles[1] = pTri[0];
	pTri[2]->m_AdjacentTriangles[2] = pTri[1];

	pTri[3]->m_AdjacentTriangles[0] = pTri[0];
	pTri[3]->m_AdjacentTriangles[1] = pTri[5];
	pTri[3]->m_AdjacentTriangles[2] = pTri[4];

	pTri[4]->m_AdjacentTriangles[0] = pTri[1];
	pTri[4]->m_AdjacentTriangles[1] = pTri[3];
	pTri[4]->m_AdjacentTriangles[2] = pTri[5];

	pTri[5]->m_AdjacentTriangles[0] = pTri[2];
	pTri[5]->m_AdjacentTriangles[1] = pTri[4];
	pTri[5]->m_AdjacentTriangles[2] = pTri[3];

	pTri[0]->m_Edges[0]->m_pPairEdge = pTri[3]->m_Edges[0];
	pTri[0]->m_Edges[1]->m_pPairEdge = pTri[1]->m_Edges[2];
	pTri[0]->m_Edges[2]->m_pPairEdge = pTri[2]->m_Edges[1];

	pTri[1]->m_Edges[0]->m_pPairEdge = pTri[4]->m_Edges[0];
	pTri[1]->m_Edges[1]->m_pPairEdge = pTri[2]->m_Edges[2];
	pTri[1]->m_Edges[2]->m_pPairEdge = pTri[0]->m_Edges[1];

	pTri[2]->m_Edges[0]->m_pPairEdge = pTri[5]->m_Edges[0];
	pTri[2]->m_Edges[1]->m_pPairEdge = pTri[0]->m_Edges[2];
	pTri[2]->m_Edges[2]->m_pPairEdge = pTri[1]->m_Edges[1];

	pTri[3]->m_Edges[0]->m_pPairEdge = pTri[0]->m_Edges[0];
	pTri[3]->m_Edges[1]->m_pPairEdge = pTri[5]->m_Edges[2];
	pTri[3]->m_Edges[2]->m_pPairEdge = pTri[4]->m_Edges[1];

	pTri[4]->m_Edges[0]->m_pPairEdge = pTri[1]->m_Edges[0];
	pTri[4]->m_Edges[1]->m_pPairEdge = pTri[3]->m_Edges[2];
	pTri[4]->m_Edges[2]->m_pPairEdge = pTri[5]->m_Edges[1];

	pTri[5]->m_Edges[0]->m_pPairEdge = pTri[2]->m_Edges[0];
	pTri[5]->m_Edges[1]->m_pPairEdge = pTri[4]->m_Edges[2];
	pTri[5]->m_Edges[2]->m_pPairEdge = pTri[3]->m_Edges[1];

	CEPATriangleComparison compare;

	for ( int i = 0; i < 6; i++ )
	{
		pTri[i]->ComputeClosestPointToOrigin(*this);

//#ifdef _DEBUG
//		pTri[i]->m_Index = m_Count++;
//
//		for ( int j = 0; j < 3; j++ )
//		{
//			assert(pTri[i]->m_AdjacentTriangles[j] == pTri[i]->m_Edges[j]->m_pPairEdge->GetEPATriangle());
//		}
//#endif

		pTri[i]->m_Index = m_Count++;

		for ( int j = 0; j < 3; j++ )
		{
			if ( !(pTri[i]->m_AdjacentTriangles[j] == pTri[i]->m_Edges[j]->m_pPairEdge->GetEPATriangle()) )
				return false;
		}

		m_Triangles.push_back(pTri[i]);
		std::push_heap(m_Triangles.begin(), m_Triangles.end(), compare);
	}

	return true;
}

bool CEPAPolytope::ExpandPolytopeWithNewPoint(const CVector3D& w, CEPATriangle* pTriangleUsedToObtainW)
{
#ifdef _DEBUG
	for ( unsigned int i = 0; i < m_Triangles.size(); i++ )
	{
		m_Triangles[i]->m_bVisible = false;
	}
#endif

	m_SilhouetteVertices.clear();
	m_SilhouetteVertices.reserve(20);
	m_SilhouetteTriangles.clear();
	m_SilhouetteTriangles.reserve(20);
	m_SilhouetteEdges.clear();
	m_SilhouetteEdges.reserve(20);

	m_Vertices.push_back(w);
	int indexVertexW = (int)m_Vertices.size() - 1;

	assert(pTriangleUsedToObtainW->IsObsolete() == false);

#ifdef _DEBUG
	pTriangleUsedToObtainW->m_bVisible = true;
#endif

	pTriangleUsedToObtainW->SetObsolete(true);

#ifdef _DEBUG	
	for ( int i = 0; i < (int)m_Triangles.size(); i++ )
	{
		if ( m_Triangles[i]->IsObsolete() )
			continue;

		int index = m_Triangles[i]->m_Index;
		bool b = m_Triangles[i]->IsVisibleFromPoint(w);
	}
#endif

	// 'Flood Fill Silhouette' algorithm to detect visible triangles and silhouette loop of edges from w.
	for ( int i = 0; i < 3; i++ )
		pTriangleUsedToObtainW->m_Edges[i]->m_pPairEdge->m_pEPATriangle->DoSilhouette(w, pTriangleUsedToObtainW->m_Edges[i], *this);
	
	assert(m_SilhouetteVertices.size() >= 3);
	assert(m_SilhouetteTriangles.size() >= 3);

	// Now, we create new triangles to patch the silhouette loop 
	int silhouetteSize = (int)m_SilhouetteVertices.size();

// Instead doing 'assert', we return false, if 'assert' fails. 
// This can happen when two objects are almost touching and numeric instability occurs. 

	//#ifdef _DEBUG
//	for ( int i = 0; i < (int)m_Triangles.size(); i++ )
//	{
//		if ( m_Triangles[i]->IsObsolete() )
//			continue;
//
//		if ( m_Triangles[i]->m_bVisible )
//			assert(m_Triangles[i]->IsVisibleFromPoint(w) == true);
//		else
//			assert(m_Triangles[i]->IsVisibleFromPoint(w) == false);
//	}
//	
//	for ( int i = 0; i < (int)silhouetteSize; i++ )
//	{
//		assert(m_SilhouetteTriangles[i]->IsVisibleFromPoint(w) == false);
//	}
//#endif

	for ( int i = 0; i < (int)m_Triangles.size(); i++ )
	{
		if ( m_Triangles[i]->IsObsolete() )
			continue;

		if ( m_Triangles[i]->m_bVisible )
			if ( m_Triangles[i]->IsVisibleFromPoint(w) != true )
				return false;
		else
			if ( m_Triangles[i]->IsVisibleFromPoint(w) != false )
				return false;
	}
	
	for ( int i = 0; i < (int)silhouetteSize; i++ )
	{
		if ( m_SilhouetteTriangles[i]->IsVisibleFromPoint(w) != false )
			return false;
	}

	std::vector<CEPATriangle*> newTriangles;
	newTriangles.reserve(silhouetteSize);

	CEPATriangleComparison compare;

	for ( int i = 0; i < silhouetteSize; i++ )
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
		newTriangles[i]->m_Edges[0]->m_pPairEdge = newTriangles[k]->m_Edges[2];

		newTriangles[i]->m_AdjacentTriangles[1] = m_SilhouetteTriangles[i];
		newTriangles[i]->m_Edges[1]->m_pPairEdge = m_SilhouetteEdges[i];
		m_SilhouetteEdges[i]->m_pPairEdge = newTriangles[i]->m_Edges[1];
		m_SilhouetteTriangles[i]->m_AdjacentTriangles[m_SilhouetteEdges[i]->m_IndexLocal] = newTriangles[i];
	}

	for ( int i = 0; i < silhouetteSize; i++ )
	{	
#ifdef _DEBUG
		newTriangles[i]->m_Index = m_Count++;
#endif

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
					CEPAEdge* edge = GetTriangles()[i]->GetEdge(j);
					assert(edge->GetIndexVertex(0) == edge->m_pPairEdge->GetIndexVertex(1));
					assert(edge->GetIndexVertex(1) == edge->m_pPairEdge->GetIndexVertex(0));
				}
			}
		}
#endif

	return true;
}

bool CEPAPolytope::IsOriginInTetrahedron(const CVector3D& p1, const CVector3D& p2, const CVector3D& p3, const CVector3D& p4)
{
    if ((p2-p1).Cross(p3-p1).Dot(p1) > 0.0 || (p2-p1).Cross(p3-p1).Dot(p4) > 0.0) 
		return false;

    if ((p4-p2).Cross(p3-p2).Dot(p2) > 0.0 || (p4-p2).Cross(p3-p2).Dot(p1) > 0.0)
        return false;

    if ((p4-p3).Cross(p1-p3).Dot(p3) > 0.0 || (p4-p3).Cross(p1-p3).Dot(p2) > 0.0) 
		return false;

    if ((p2-p4).Cross(p1-p4).Dot(p4) > 0.0 || (p2-p4).Cross(p1-p4).Dot(p3) > 0.0) 
		return false;

    return true;
}