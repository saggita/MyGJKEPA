#pragma once

#include <cassert>
#include <vector>
#include "Vector3D.h"
#include "EPAEdge.h"

class CEPATriangle
{
friend class CEPAPolytope;

public:
	CEPATriangle();
	CEPATriangle(int indexVertex0, int indexVertex1, int indexVertex2);
	~CEPATriangle();

private:
	int m_IndicesVertex[3];
	CEPATriangle* m_AdjacentTriangles[3];
	CEPAEdge* m_Edges[3];
	bool m_bObsolete;
	btScalar m_Det;
	
	CVector3D m_ClosestPointToOrigin; 

	btScalar m_Lambda1; 
	btScalar m_Lambda2;

	// squared distance to origin
	btScalar m_DistSqr; // = m_ClosestPointToOrigin.LenghSqr()
	
public:

#ifdef _DEBUG
	int m_Index;
	bool m_bVisible;
#endif

	int GetIndexVertex(int i) const 
	{ 
		assert(0 <= i && i < 3);
		return m_IndicesVertex[i]; 
	}

	CEPAEdge* GetEdge(int i)
	{
		assert(0 <= i && i < 3);
		return m_Edges[i];
	}

	void SetAdjacentEdge(int index, CEPAEdge& EPAEdge);
	btScalar GetDistSqr() const { return m_DistSqr; }
	bool IsObsolete() const { return m_bObsolete; }
	void SetObsolete(bool bObsolete) { m_bObsolete = bObsolete; }	
	const CVector3D& GetClosestPoint() const { return m_ClosestPointToOrigin; }
	bool IsClosestPointInternal() const;
	bool IsVisibleFromPoint(const CVector3D& point) const;
	bool ComputeClosestPointToOrigin(const CEPAPolytope& EPAPolytope);
	CVector3D GetClosestPointToOriginInSupportPntSpace(const std::vector<CVector3D>& supportPoints) const;
	bool DoSilhouette(const CVector3D& w, CEPAEdge* edge, CEPAPolytope& EPAPolytope);
		
	bool operator<(const CEPATriangle& other) const;
};

class CEPATriangleComparison 
{
public:
	bool operator() (const CEPATriangle* pTriA, const CEPATriangle* pTriB) 
	{
		return (pTriA->GetDistSqr() > pTriB->GetDistSqr());
    }
};
