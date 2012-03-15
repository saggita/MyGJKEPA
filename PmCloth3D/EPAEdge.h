#pragma once

#include "Vector3D.h"

class CEPATriangle;
class CTrianglesStore;

//==================
class CEPAEdge
//==================
{
public:
	CEPAEdge();
	CEPAEdge(CEPATriangle* pEPATriangle, int index);
	~CEPAEdge();

private:
	int m_Index; // 0, 1 or 2
	CEPATriangle* m_pEPATriangle;

public:
	int GetIndex() const { return m_Index; };
	CEPATriangle* GetEPATriangle() const { return m_pEPATriangle; }
	int GetSourceVertexIndex() const;
	int GetTargetVertexIndex() const;
	bool DoSilhouette(const CVector3D* pVertices, int index, CTrianglesStore& triangleStore);
};

