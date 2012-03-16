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

	std::vector<int> m_SilhouetteVertices;
	std::vector<CEPATriangle*> m_SilhouetteTriangles;
	std::vector<CEPAEdge*> m_SilhouetteEdges;
	std::vector<CEPATriangle*> m_VisibleTriangles;
	std::vector<CVector3D> m_SupportPointsA; // support points from object A in local coordinate
	std::vector<CVector3D> m_SupportPointsB; // support points from object B in local coordinate

	int m_Count;

public:
	void Clear();
	std::vector<CEPATriangle*>& GetTriangles() { return m_Triangles; }
	const std::vector<CEPATriangle*>& GetTriangles() const { return m_Triangles; }
	std::vector<CVector3D>& GetVertices() { return m_Vertices; }
	const std::vector<CVector3D>& GetVertices() const { return m_Vertices; }

	CEPATriangle* PopAClosestTriangleToOriginFromHeap();

	bool AddTetrahedron(const CVector3D& p0, const CVector3D& p1, const CVector3D& p2, const CVector3D& p3);
	bool ExpandPolytopeWithNewPoint(const CVector3D& w, CEPATriangle* pTriangleUsedToObtainW);

	static bool IsOriginInTetrahedron(const CVector3D& p1, const CVector3D& p2, const CVector3D& p3, const CVector3D& p4);
};

