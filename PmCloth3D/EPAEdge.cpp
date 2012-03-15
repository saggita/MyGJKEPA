#include <cassert>
#include "EPAEdge.h"
#include "NarrowPhaseGJK.h"

CEPAEdge::CEPAEdge()
{

}

CEPAEdge::CEPAEdge(CEPATriangle* pEPATriangle, int index) : m_pEPATriangle(pEPATriangle), m_Index(index) 
{ 
	assert(index >= 0 && index < 3); 
}

CEPAEdge::~CEPAEdge() 
{
}

int CEPAEdge::GetSourceVertexIndex() const
{
	return (*m_pEPATriangle).GetIndexVertex(m_Index);
}

int CEPAEdge::GetTargetVertexIndex() const
{
	return (*m_pEPATriangle)[(m_Index + 1 ) % 3];
}

bool CEPAEdge::DoSilhouette(const CVector3D* pVertices, int index, CTrianglesStore& triangleStore)
{
	if ( !m_pEPATriangle->IsObsolete() )
	{
		if ( m_pEPATriangle->IsVisibleFromPoint((pVertices[index])) ) // the owner triangle is visible.
		{
			m_pEPATriangle->SetObsolete(true);

			unsigned int numTriangles = triangleStore.getNbTriangles();

			if ( !m_pEPATriangle->GetAdjacentEdge((m_Index + 1 ) % 3).DoSilhouette(pVertices, index, triangleStore) )
			{

			}
		}
		else // the owner triangle is NOT visible.
		{
			CEPATriangle* triangle = triangleStore.newTriangle(pVertices, index, GetTargetVertexIndex(), GetSourceVertexIndex());

			if ( triangle )
			{
				halfLink(CEPAEdge(triangle, 1), *this);
				return true;
			}

			return false;
		}

	}

	return true;
}