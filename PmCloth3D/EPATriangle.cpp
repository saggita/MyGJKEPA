#pragma once

#include <cstdio>
#include <cassert>
#include "EPATriangle.h"
#include "EPAPolytope.h"

CEPATriangle::CEPATriangle()
{
}

CEPATriangle::CEPATriangle(int indexVertex0, int indexVertex1, int indexVertex2) : m_bObsolete(false)
{
	m_IndicesVertex[0] = indexVertex0;
	m_IndicesVertex[1] = indexVertex1;
	m_IndicesVertex[2] = indexVertex2;
}

CEPATriangle::~CEPATriangle()
{
}

CEPAEdge& CEPATriangle::GetAdjacentEdge(int index)
{
	assert(0 <= index && index < 3);
	return m_AdjacentEdges[index];
}

void CEPATriangle::SetAdjacentEdge(int index, CEPAEdge& EPAEdge)
{
	assert(0 <= index && index < 3);
	m_AdjacentEdges[index] = EPAEdge;
}

bool CEPATriangle::IsClosestPointInternal() const
{
	return ( m_Lambda1 >= 0.0 && m_Lambda2 >= 0.0 && (m_Lambda1 + m_Lambda2) <= m_Det);
}

bool CEPATriangle::IsVisibleFromPoint(const CVector3D& point) const
{
	//return (m_ClosestPointToOrigin.Dot(point - m_ClosestPointToOrigin) > 0);
	//return point.Dot(m_ClosestPointToOrigin) >= m_ClosestPointToOrigin.LengthSqr();
	return point.Dot(m_ClosestPointToOrigin) >= m_DistSqr;
}

// TODO: Revisit this.
bool CEPATriangle::ComputeClosestPointToOrigin(const CVector3D* vertices)
{
	const CVector3D& p0 = vertices[m_IndicesVertex[0]];
	const CVector3D& p1 = vertices[m_IndicesVertex[1]];
	const CVector3D& p2 = vertices[m_IndicesVertex[2]];

	CVector3D v1 = p1 - p0;
	CVector3D v2 = p2 - p0;
	
	double v1Dotv1 = v1.Dot(v1);
    double v1Dotv2 = v1.Dot(v2);
    double v2Dotv2 = v2.Dot(v2);
    double p0Dotv1 = p0.Dot(v1);
    double p0Dotv2 = p0.Dot(v2);

    // Compute determinant
    m_Det = v1Dotv1 * v2Dotv2 - v1Dotv2 * v1Dotv2;

    // Compute lambda values
    m_Lambda1 = p0Dotv2 * v1Dotv2 - p0Dotv1 * v2Dotv2;
    m_Lambda2 = p0Dotv1 * v1Dotv2 - p0Dotv2 * v1Dotv1;

    // If the determinant is positive
    if ( m_Det > 0.0 ) 
	{
        // Compute the closest point v
        m_ClosestPointToOrigin = p0 + 1.0 / m_Det * (m_Lambda1 * v1 + m_Lambda2 * v2);

        // Compute the square distance of closest point to the origin
		m_DistSqr = m_ClosestPointToOrigin.LengthSqr();

        return true;
    }

    return false;
}

bool CEPATriangle::ComputeClosestPointToOrigin(const CEPAPolytope& EPAPolytope)
{
	const CVector3D& p0 = EPAPolytope.m_Vertices[m_IndicesVertex[0]];
	const CVector3D& p1 = EPAPolytope.m_Vertices[m_IndicesVertex[1]];
	const CVector3D& p2 = EPAPolytope.m_Vertices[m_IndicesVertex[2]];

	CVector3D v1 = p1 - p0;
	CVector3D v2 = p2 - p0;
	
	double v1Dotv1 = v1.Dot(v1);
    double v1Dotv2 = v1.Dot(v2);
    double v2Dotv2 = v2.Dot(v2);
    double p0Dotv1 = p0.Dot(v1);
    double p0Dotv2 = p0.Dot(v2);

    // Compute determinant
    m_Det = v1Dotv1 * v2Dotv2 - v1Dotv2 * v1Dotv2;

    // Compute lambda values
    m_Lambda1 = p0Dotv2 * v1Dotv2 - p0Dotv1 * v2Dotv2;
    m_Lambda2 = p0Dotv1 * v1Dotv2 - p0Dotv2 * v1Dotv1;

    // If the determinant is positive
    if ( m_Det > 0.0 ) 
	{
        // Compute the closest point v
        m_ClosestPointToOrigin = p0 + 1.0 / m_Det * (m_Lambda1 * v1 + m_Lambda2 * v2);

        // Compute the square distance of closest point to the origin
		m_DistSqr = m_ClosestPointToOrigin.LengthSqr();

        return true;
    }

    return false;
}

CVector3D CEPATriangle::ComputeClosestPointOfObject(const CVector3D* supportPointsOfObject) const
{
	const CVector3D& p0 = supportPointsOfObject[m_IndicesVertex[0]];
	const CVector3D& p1 = supportPointsOfObject[m_IndicesVertex[1]];
	const CVector3D& p2 = supportPointsOfObject[m_IndicesVertex[2]];

	return p0 + (1.0/m_Det) * (m_Lambda1 * (p1 - p0) + m_Lambda2 * (p2 - p0));
}

bool CEPATriangle::DoSilhouette(const CVector3D* pVertices, int index, CTrianglesStore& triangleStore)
{
	unsigned int numTrianglesPrev = triangleStore.getNbTriangles();

	SetObsolete(true);

	bool bDone = m_AdjacentEdges[0].DoSilhouette(pVertices, index, triangleStore) &&
				 m_AdjacentEdges[1].DoSilhouette(pVertices, index, triangleStore) &&
				 m_AdjacentEdges[2].DoSilhouette(pVertices, index, triangleStore);

	if ( bDone )
	{
		int i,j;

        for (i=numTrianglesPrev, j=triangleStore.getNbTriangles()-1; i != triangleStore.getNbTriangles(); j = i++) 
		{
            CEPATriangle* triangle = &triangleStore[i];
            halfLink(triangle->GetAdjacentEdge(1), CEPAEdge(triangle, 1));

            if (!link(CEPAEdge(triangle, 0), CEPAEdge(&triangleStore[j], 2))) 
                return false;
        }
	}

	return bDone;
}

int CEPATriangle::GetNextVertexIndexCCW(int indexPivotVertex, int* indexEdge)
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		if ( m_IndicesVertex[i] == indexPivotVertex )
			break;
	}

	*indexEdge = i;
	i = ( i + 1) % 3;

	return m_IndicesVertex[i];
}

// Before calling this, m_bVisited of all triangles should be initialized as true;
bool CEPATriangle::DoSilhouette(const CVector3D& w, int indexPivotVertex, const CEPATriangle* callerTriangle, CEPAPolytope& EPAPolytope)
{
#ifdef _DEBUG
	int index = m_Index;
#endif

	if ( IsVisited() )
	{
		if ( !IsVisibleFromPoint(w) )
		{
			EPAPolytope.m_SilhouetteVertices.push_back(indexPivotVertex);
			EPAPolytope.m_SilhouetteTriangles.push_back(this);
		}

		return false;
	}

	SetVisited(true);

	if ( IsVisibleFromPoint(w) )
	{
		int indexEdge;
		int indexNextVertex = GetNextVertexIndexCCW(indexPivotVertex, &indexEdge);

		if ( m_AdjacentTriangles[indexEdge] != callerTriangle )
			m_AdjacentTriangles[indexEdge]->DoSilhouette(w, indexPivotVertex, this, EPAPolytope);

		indexPivotVertex = indexNextVertex;
		indexNextVertex = GetNextVertexIndexCCW(indexNextVertex, &indexEdge);

		if ( m_AdjacentTriangles[indexEdge] != callerTriangle )
			m_AdjacentTriangles[indexEdge]->DoSilhouette(w, indexPivotVertex, this, EPAPolytope);

		indexPivotVertex = indexNextVertex;
		indexNextVertex = GetNextVertexIndexCCW(indexNextVertex, &indexEdge);

		if ( m_AdjacentTriangles[indexEdge] != callerTriangle )
			m_AdjacentTriangles[indexEdge]->DoSilhouette(w, indexPivotVertex, this, EPAPolytope);

		SetObsolete(true);
		EPAPolytope.m_VisibleTriangles.push_back(this);
		m_bVisible = true;
	}
	else
	{
		EPAPolytope.m_SilhouetteVertices.push_back(indexPivotVertex);
		EPAPolytope.m_SilhouetteTriangles.push_back(this);
	}

	return true;
}

int CEPATriangle::operator[](int i) const
{
	assert(i >= 0 && i < 3);
	return m_IndicesVertex[i];
}

bool CEPATriangle::operator<(const CEPATriangle& other) const
{
	return m_DistSqr > other.m_DistSqr;
}

//==================
// CTrianglesStore
//==================
// Return the last triangle
CEPATriangle& CTrianglesStore::last() {
    assert(nbTriangles > 0);
    return triangles[nbTriangles - 1];
}

// Create a new triangle
CEPATriangle* CTrianglesStore::newTriangle(const CVector3D* vertices, int v0, int v1, int v2) {
    CEPATriangle* newTriangle = NULL;

    // If we have not reach the maximum number of triangles
    if (nbTriangles != MAX_TRIANGLES) {
        newTriangle = &triangles[nbTriangles++];
        newTriangle = new CEPATriangle(v0, v1, v2);
		if (!newTriangle->ComputeClosestPointToOrigin(vertices)) {
            nbTriangles--;
            newTriangle = NULL;
        }
    }

    // Return the new triangle
    return newTriangle;
}

// Access operator
CEPATriangle& CTrianglesStore::operator[](int i) 
{
    return triangles[i];
}

