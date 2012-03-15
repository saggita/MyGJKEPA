#pragma once

#include <vector>
#include "EPATriangle.h"

class CEPAPolytope
{
friend class CEPATriangle;

public:
	CEPAPolytope(void);
	~CEPAPolytope(void);

private:
	std::vector<CVector3D> m_Vertices;

	// array of CEPATriangle constructing polytope. 
	std::vector<CEPATriangle*> m_Triangles;
	//bool DoSilhouette(const CVector3D& w, const CEPATriangle* pTriangle);

	std::vector<int> m_SilhouetteVertices;
	std::vector<CEPATriangle*> m_SilhouetteTriangles;
	std::vector<CEPATriangle*> m_VisibleTriangles;

	int m_Count;

public:
	void Clear();
	std::vector<CEPATriangle*>& GetTriangles() { return m_Triangles; }
	const std::vector<CEPATriangle*>& GetTriangles() const { return m_Triangles; }
	CEPATriangle* PopAClosestTriangleToOriginFromHeap();

	bool AddTetrahedron(const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, const CVector3D& p3);
	bool AddPoint(const CVector3D& w, CEPATriangle* pTriangleUsedToObjtainW);
};

