#pragma once

#include <cassert>
#include "Vector3D.h"

class CEPATriangle;

class CEPAEdge
{
friend class CEPATriangle;
friend class CEPAPolytope;

public:
	CEPAEdge();
	CEPAEdge(CEPATriangle* pEPATriangle, int indexLocal, int indexVertex0, int indexVertex1);
	~CEPAEdge();

private:
	int m_IndexLocal; // 0, 1 or 2 From m_pEPATriangle's point of view, 0, 1, 2 winding order is counter clockwise.

	int m_IndexVertex[2]; // m_IndexVertex[0] is index of starting vertex. m_IndexVertex[1] is index of ending vertex. 

public:
	CEPATriangle* m_pEPATriangle; // pointer to owner triangle
	CEPAEdge* m_pPairEdge;


	int GetIndexLocal() const { return m_IndexLocal; }
	CEPATriangle* GetEPATriangle() const { return m_pEPATriangle; }

	int GetIndexVertex(int i)
	{
		assert( i == 0 || i == 1);
		return m_IndexVertex[i];
	}
};

