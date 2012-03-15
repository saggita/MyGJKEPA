#pragma once

#include "Cloth3D.h"
#include "NarrowPhaseGJK.h"

class CBVHMasterTree;
struct BVHCollisionResult;

class CClothSim3D
{
public:
	struct NarrowCollisionResult
	{
		enum CollisionType { VERTEX_TRIANGLE, EDGE_EDGE };
	
		CollisionType collisionType;

		// If collisionType == VERTEX_TRIANGLE, indexes[0] is the index of vertex in cloths[0]. 
		// indexes[1] is the index of triangle in cloths[1].
		// If collisionType == EDGE_EDGE, indexes[0] is the index of edge in cloths[0].
		// indexes[1] is the index of edge in cloths[1].
		CCloth3D* cloths[2];
		int indexes[2];

		union
		{
			double a, b, c; // Barycentric coordinates when collisionType == VERTEX_TRIANGLE
			double p, q; // Barycentric coordinates when collisionType == EDGE_EDGE
		};
		
		CVector3D normal;
	};

	CClothSim3D(void);
	virtual ~CClothSim3D(void);

private:
	CClothSim3D(const CClothSim3D& other) {}; // private copy constructor

public:
	std::vector<CCloth3D*> m_ClothList;
	CNarrowPhaseGJK* m_pNarrorPhase;
	CBVHMasterTree* m_pBVHMasterTree;

	double m_dt;
	int m_NumIterForGlobalCol; 
	int m_NumOfUnresolvedCols;
	int m_NumOfTriTriCols;
	CVector3D m_Gravity;
	double m_BroadPhaseTolerance; 

	int m_Substeps; // = 1

public:
	void Create();
	void AddCloth(CCloth3D* pCloth);
	void ClearAll();
	unsigned int Update(double dt);
	void Render() const;
	void SetGravity(const CVector3D& gravity);
	const CVector3D& GetGravity() const;

	const std::vector<CCloth3D*>& GetCloths() const { return m_ClothList; }
	void SetSubsteps(int substeps) { m_Substeps = substeps; }
	int GetSubsteps() const { return m_Substeps; }

	// For debug
	void ShowBV(bool bShowBV);

protected:
	unsigned int SubsUpdate(double dt);
	
private:
	CClothSim3D& operator=(const CClothSim3D& other) { return *this; }; // private assign operator. This means you cannot use assign operator.
};