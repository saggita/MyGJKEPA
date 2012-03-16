#include <cassert>
#include "EPAEdge.h"
#include "NarrowPhaseGJK.h"

CEPAEdge::CEPAEdge()
{

}

CEPAEdge::CEPAEdge(CEPATriangle* pEPATriangle, int indexLocal, int indexVertex0, int indexVertex1) : m_pEPATriangle(pEPATriangle), m_IndexLocal(indexLocal) 
{ 
	assert(indexLocal >= 0 && indexLocal < 3); 

	m_IndexVertex[0] = indexVertex0;
	m_IndexVertex[1] = indexVertex1;
}

CEPAEdge::~CEPAEdge() 
{
}

int CEPAEdge::GetSourceVertexIndex() const
{
	return (*m_pEPATriangle).GetIndexVertex(m_IndexLocal);
}

int CEPAEdge::GetTargetVertexIndex() const
{
	return (*m_pEPATriangle)[(m_IndexLocal + 1 ) % 3];
}

bool CEPAEdge::DoSilhouette(const CVector3D* pVertices, int index, CTrianglesStore& triangleStore)
{
	if ( !m_pEPATriangle->IsObsolete() )
	{
		if ( m_pEPATriangle->IsVisibleFromPoint((pVertices[index])) ) // the owner triangle is visible.
		{
			m_pEPATriangle->SetObsolete(true);

			unsigned int numTriangles = triangleStore.getNbTriangles();

			if ( !m_pEPATriangle->GetAdjacentEdge((m_IndexLocal + 1 ) % 3).DoSilhouette(pVertices, index, triangleStore) )
			{

			}
		}
		else // the owner triangle is NOT visible.
		{
			CEPATriangle* triangle = triangleStore.newTriangle(pVertices, index, GetTargetVertexIndex(), GetSourceVertexIndex());

			if ( triangle )
			{
				//halfLink(CEPAEdge(triangle, 1), *this);
				return true;
			}

			return false;
		}

	}

	return true;
}