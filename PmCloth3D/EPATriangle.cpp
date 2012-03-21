#pragma once

#include <cstdio>
#include <cassert>
#include "EPATriangle.h"
#include "EPAPolytope.h"

CEPATriangle::CEPATriangle() : m_bObsolete(false)
{
	for ( int i = 0; i < 3; i++ )
		m_Edges[i] = NULL;

#ifdef _DEBUG
	m_Index = 0;
	m_bVisible = false;
#endif
}

// vertex 0, 1 and 2 should be formed in counter clockwise
CEPATriangle::CEPATriangle(int indexVertex0, int indexVertex1, int indexVertex2) : m_bObsolete(false)
{
	m_IndicesVertex[0] = indexVertex0;
	m_IndicesVertex[1] = indexVertex1;
	m_IndicesVertex[2] = indexVertex2;

	for ( unsigned int i = 0; i < 3; i++ )
		m_Edges[i] = new CEPAEdge(this, i, m_IndicesVertex[i], m_IndicesVertex[(i+1) % 3]);
}

CEPATriangle::~CEPATriangle()
{
	for ( int i = 0; i < 3; i++ )
		delete m_Edges[i]; // deleting NULL is not harmful. So we don't check if m_Edges[i] is NULL or not
}

bool CEPATriangle::IsClosestPointInternal() const
{
	return ( m_Lambda1 >= 0.0 && m_Lambda2 >= 0.0 && (m_Lambda1 + m_Lambda2) <= m_Det);
}

bool CEPATriangle::IsVisibleFromPoint(const CVector3D& point) const
{
	return point.Dot(m_ClosestPointToOrigin) >= m_DistSqr;
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

    m_Det = v1Dotv1 * v2Dotv2 - v1Dotv2 * v1Dotv2;
    m_Lambda1 = p0Dotv2 * v1Dotv2 - p0Dotv1 * v2Dotv2;
    m_Lambda2 = p0Dotv1 * v1Dotv2 - p0Dotv2 * v1Dotv1;

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

CVector3D CEPATriangle::GetClosestPointToOriginInSupportPntSpace(const std::vector<CVector3D>& supportPoints) const
{
	const CVector3D* sp[3];
	
	for (int i = 0; i < 3; i++ )
		sp[i] = &supportPoints[m_IndicesVertex[i]];

	return (*sp[0]) + (1.0/m_Det) * (m_Lambda1 * ((*sp[1]) - (*sp[0])) + m_Lambda2 * ((*sp[2]) - (*sp[0])));
}

// Please note that edge doesn't belong to this triangle. It is from the neighbor triangle.
// edge->m_pEPATriangle is a neighbor triangle which called this function. 
// edge->m_pPairEdge belongs to this triangle. 
bool CEPATriangle::DoSilhouette(const CVector3D& w, CEPAEdge* edge, CEPAPolytope& EPAPolytope)
{
#ifdef _DEBUG
	int index = m_Index;
#endif

	assert(edge != NULL);
	assert(edge->m_pPairEdge != NULL);
	assert(edge->m_pEPATriangle != NULL);
	
	if ( m_bObsolete )
		return true;

	if ( !IsVisibleFromPoint(w) ) // if this triangle is not visible from point w
	{
		int indexVertex0 = edge->m_IndexVertex[0];
		EPAPolytope.m_SilhouetteVertices.push_back(indexVertex0);
		EPAPolytope.m_SilhouetteTriangles.push_back(this);
		EPAPolytope.m_SilhouetteEdges.push_back(edge->m_pPairEdge);
		return true;
	}
	else // if visible
	{
#ifdef _DEBUG
		m_bVisible = true;
#endif

		m_bObsolete = true;
		CEPAEdge* myEdge = edge->m_pPairEdge;

		assert(m_Edges[myEdge->m_IndexLocal] == myEdge);

		int indexNextEdgeCCW = (myEdge->m_IndexLocal + 1) % 3;
		assert(0 <= indexNextEdgeCCW && indexNextEdgeCCW < 3);
		m_Edges[indexNextEdgeCCW]->m_pPairEdge->m_pEPATriangle->DoSilhouette(w, m_Edges[indexNextEdgeCCW], EPAPolytope);

		indexNextEdgeCCW = (indexNextEdgeCCW + 1) % 3;
		assert(0 <= indexNextEdgeCCW && indexNextEdgeCCW < 3);
		m_Edges[indexNextEdgeCCW]->m_pPairEdge->m_pEPATriangle->DoSilhouette(w, m_Edges[indexNextEdgeCCW], EPAPolytope);
	}

	return true;
}

bool CEPATriangle::operator<(const CEPATriangle& other) const
{
	return m_DistSqr > other.m_DistSqr;
}
