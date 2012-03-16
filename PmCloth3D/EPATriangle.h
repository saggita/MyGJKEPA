#pragma once

#include "Vector3D.h"
#include "EPAEdge.h"

static const unsigned int MAX_TRIANGLES = 200;     // Maximum number of triangles
static const unsigned int MAX_SUPPORT_POINTS = 100;    // Maximum number of support points of the polytope
static const unsigned int MAX_FACETS = 200;            // Maximum number of facets of the polytope

class CTrianglesStore;

//================
class CEPATriangle
//================
{
friend class CEPAPolytope;

public:
	CEPATriangle();
	CEPATriangle(int indexVertex0, int indexVertex1, int indexVertex2);
	~CEPATriangle();

private:
	int m_IndicesVertex[3];
	CEPAEdge m_AdjacentEdges[3];
	CEPATriangle* m_AdjacentTriangles[3];
	CEPAEdge* m_Edges[3];
	bool m_bObsolete;
	double m_Det;
	bool m_bVisited;
	
	CVector3D m_ClosestPointToOrigin; 

	double m_Lambda1; 
	double m_Lambda2;

	// squared distance to origin
	double m_DistSqr; // = m_ClosestPointToOrigin.LenghSqr()

	int GetNextVertexIndexCCW(int indexPivotVertex, int* indexEdge);

public:
	int m_Index;
	bool m_bVisible;

	int GetIndexVertex(int i) const { return m_IndicesVertex[i]; }
	CEPAEdge& GetAdjacentEdge(int index);
	CEPAEdge* GetEdge(int i)
	{
		assert(0 <= i && i < 3);
		return m_Edges[i];
	}

	void SetAdjacentEdge(int index, CEPAEdge& EPAEdge);
	double GetDistSqr() const { return m_DistSqr; }
	bool IsObsolete() const { return m_bObsolete; }
	void SetObsolete(bool bObsolete) { m_bObsolete = bObsolete; }
	bool IsVisited() const { return m_bVisited; }
	void SetVisited(bool bVisited) { m_bVisited = bVisited; }
	const CVector3D& GetClosestPoint() const { return m_ClosestPointToOrigin; }
	bool IsClosestPointInternal() const;
	bool IsVisibleFromPoint(const CVector3D& point) const;
	bool ComputeClosestPointToOrigin(const CVector3D* vertices);
	bool ComputeClosestPointToOrigin(const CEPAPolytope& EPAPolytope);
	CVector3D ComputeClosestPointOfObject(const CVector3D* supportPointsOfObject) const;
	bool DoSilhouette(const CVector3D* pVertices, int index, CTrianglesStore& triangleStore);
	bool DoSilhouette(const CVector3D& w, int indexPivotVertex, const CEPATriangle* callerTriangle, CEPAPolytope& EPAPolytope);
	bool DoSilhouette(const CVector3D& w, CEPAEdge* edge, CEPAPolytope& EPAPolytope);

	int operator[](int i) const;  
	bool operator<(const CEPATriangle& other) const;
	
	friend bool link(const CEPAEdge& edge0, const CEPAEdge& edge1);
	friend void halfLink(const CEPAEdge& edge0, const CEPAEdge& edge1);
};

class CEPATriangleComparison 
{
public:
	bool operator() (const CEPATriangle* pTriA, const CEPATriangle* pTriB) 
	{
		return (pTriA->GetDistSqr() > pTriB->GetDistSqr());
    }
};

//===================
class CTrianglesStore 
//===================
{
    private:
        CEPATriangle triangles[MAX_TRIANGLES];       // Triangles
        int nbTriangles;                            // Number of triangles
        
    public:
		CTrianglesStore() : nbTriangles(0) {}                           // Constructor
		~CTrianglesStore() {}                          // Destructor

		void clear() { nbTriangles = 0; };                               // Clear all the storage
		int getNbTriangles() const { return nbTriangles; }                 // Return the number of triangles
		void setNbTriangles(int backup) { nbTriangles = backup; }            // Set the number of triangles
        CEPATriangle& last();                        // Return the last triangle

        CEPATriangle* newTriangle(const CVector3D* vertices, int v0, int v1, int v2);  // Create a new triangle

        CEPATriangle& operator[](int i);     // Access operator
};