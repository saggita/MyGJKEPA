#pragma once

#include "CollisionObject.h"

class CMinkowskiSum
{
public:
	CMinkowskiSum(void);
	~CMinkowskiSum(void);

	std::vector<CVector3D> m_Vertices;	
	std::vector<CVector3D> m_Normals;
	std::vector<CTriangleFace> m_Faces;
	std::vector<CEdge> m_Edges;

public:
	// Move objB around objA. The other way around would create the same result. 
	bool Create(const CCollisionObject& objA, const CCollisionObject& objB);
	void Render(bool bWireframe = false) const;
};

