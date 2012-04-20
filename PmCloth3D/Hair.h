#pragma once

#include <vector>
#include <list>
#include <assert.h>

#include <iostream>
#include <fstream>

#include "Point3D.h"
#include "Vector3D.h"
#include "TriangleCloth3D.h"

//----------------------------------------------
// CSpringHair
//----------------------------------------------
struct CSpringHair
{
	friend class CHair;

public:
	CSpringHair() 
	{ 
		m_IndexVrx[0] = -1; 
		m_IndexVrx[1] = -1; 
		m_Index = -1;
		m_Coloring = -1;
	}

	CSpringHair(int indexVrx0, int indexVrx1) 
	{ 
		m_IndexVrx[0] = indexVrx0; 
		m_IndexVrx[1] = indexVrx1;
		m_Index = -1; 
		m_Coloring = -1;
	}

	CSpringHair(const CSpringHair& other) 
	{
		for ( int i = 0; i < 2; i++ )
		{
			m_IndexVrx[i] = other.m_IndexVrx[i];
		}

		m_Index = other.m_Index;
		m_RestLength = other.m_RestLength;
		m_Coloring = other.m_Coloring;
	}
	
	virtual ~CSpringHair() { }

protected:
	int m_Index;
	int m_IndexVrx[2];
	btScalar m_RestLength;

public:
	int m_Coloring;

	int GetVertexIndex(int i) const 
	{
		assert( 0 <= i && i <= 1 );

		return m_IndexVrx[i];
	}

	btScalar GetRestLength() const { return m_RestLength; }
	void SetRestLength(btScalar restLength) { m_RestLength = restLength; }

	int GetIndex() const { return m_Index; }
	void SetIndex(int index) { m_Index = index; }

	int GetTheOtherVertexIndex(int indexVert)
	{
		assert(indexVert == m_IndexVrx[0] || indexVert == m_IndexVrx[1]);

		if ( indexVert == m_IndexVrx[0] )
			return m_IndexVrx[1];
		else
			return m_IndexVrx[0];
	}

	bool operator==(const CSpringHair& other)
	{
		if ( ( m_IndexVrx[0] == other.m_IndexVrx[0] && m_IndexVrx[1] == other.m_IndexVrx[1] ) ||
			 ( m_IndexVrx[0] == other.m_IndexVrx[1] && m_IndexVrx[1] == other.m_IndexVrx[0] ) )
			 return true;
		else
			return false;
	}

	CSpringHair& operator=(const CSpringHair& other)
	{
		for ( int i = 0; i < 2; i++ )
		{
			m_IndexVrx[i] = other.m_IndexVrx[i];
		}

		m_Index = other.m_Index;
		m_RestLength = other.m_RestLength;
		m_Coloring = other.m_Coloring;

		return (*this);
	}
};

struct CClothPin;

//----------------------------------------------
// CVertexHair
//----------------------------------------------
struct CVertexHair
{
public:
	CVertexHair() : m_InvMass(1.0), m_PinIndex(-1)
	{
	}

	virtual ~CVertexHair() {};

	int m_Index;
	CVector3D m_Pos;
	CVector3D m_PosNext;
	CVector3D m_Vel;
	btScalar m_InvMass; // = 1.0 / m_Mass. In case mass is infinite, m_InvMass is zero and m_Mass doesn't have any meaning.
					  // Currently infinite mass is not supported. CClothPin should be used to pin cloth vertex.
	CVector3D m_Accel;
	int m_PinIndex;

	// array of indexes of stretch springs connected to this vertex 
	std::vector<int> m_StrechSpringIndexes; 

	// array of indexes of bend springs connected to this vertex 
	std::vector<int> m_BendSpringIndexes; 

public:
	int GetIndex() const { return m_Index; }
	void SetIndex(int index) { m_Index = index; }
};


#include "ICollidable.h"
#include "CollisionObject.h"

class CHair : public ICollidable
{
public:
	CHair(void);
	CHair(const CHair& other);
	virtual ~CHair(void);

protected:
	btScalar m_dt;
	btScalar m_Kst; // stretch force
	btScalar m_Ksh; // shear force
	btScalar m_Kb; // bending force
	btScalar m_Kd;
	btScalar m_Mu; // friction
	CVector3D m_Gravity;
	bool m_bDeformable;

	std::vector<CVertexHair> m_VertexArray;
	std::vector<CSpringHair> m_StrechSpringArray;
	std::vector<CSpringHair> m_BendSpringArray;

	std::vector<int> m_BatchSpringIndexArray;

public:
	unsigned int m_NumIter;
	bool m_bEqualVertexMass;
	int m_NumIterForConstraintSolver;

	virtual bool Load(const char* filename);
	virtual void Initialize();
	btScalar GetKst() const { return m_Kst; };
	btScalar GetKsh() const { return m_Ksh; };
	btScalar GetKb() const { return m_Kb; };
	btScalar GetFrictionCoef() const { return m_Mu; }
	void SetKst(btScalar Kst) { m_Kst = Kst; };
	void SetKsh(btScalar Ksh) { m_Ksh = Ksh; };
	void SetKb(btScalar Kb) { m_Kb = Kb; };
	void SetFrictionCoef(btScalar mu) { assert(mu >= 0 && mu <= 1.0); m_Mu = mu; }
	btScalar Getdt() const { return m_dt; } 
	void Setdt(btScalar dt) { m_dt = dt; }
	void SetGravity(const CVector3D& gravity);
	const CVector3D& GetGravity() const;
	void SetMassDensity(btScalar massDensity);
	void SetVertexMass(btScalar vertexMass);
	void SetTotalMass(btScalar totalMass);
	void SetNumIterForConstraintSolver(int numIter) { m_NumIterForConstraintSolver = numIter; }

	void AddPin(int vertexIndex);

	std::vector<CVertexHair>& GetVertexArray() { return m_VertexArray; }
	const std::vector<CVertexHair>& GetVertexArray() const { return m_VertexArray; }

	std::vector<CSpringHair>& GetStrechSpringArray() { return m_StrechSpringArray; }
	const std::vector<CSpringHair>& GetStrechSpringArray() const { return m_StrechSpringArray; }

	std::vector<CSpringHair>& GetBendSpringArray() { return m_BendSpringArray; }
	const std::vector<CSpringHair>& GetBendSpringArray() const { return m_BendSpringArray; }
	
	bool IsDeformable() const { return m_bDeformable; }
	void SetDeformable(bool bDeformable) { m_bDeformable = bDeformable; }

	void Clear();
	
	void GenerateBatches();

	virtual bool Integrate(btScalar dt);
	virtual bool AdvancePosition(btScalar dt);
	
	virtual void Render();
	int RenderBatch(int i) const;

	virtual bool ResolveCollision(CCollisionObject& convexObject, btScalar dt);

	virtual CCollisionObject* GetCollisionObject() { return NULL; }
	virtual const CCollisionObject* GetCollisionObject() const { return NULL; }

	virtual void TranslateW(btScalar x, btScalar y, btScalar z);

protected:
	void FillSpringArray();
	void ApplyGravity(btScalar dt);
	void ApplyForces(btScalar dt);
	void ClearForces();	
	void ComputeNextVertexPositions(btScalar dt);
	btScalar CalcConstraint(int indexEdge, int indexVertex, btScalar dt, CVector3D* pGradientOfConstraint = NULL);
	void EnforceEdgeConstraints(btScalar dt);
	void EnforceEdgeConstraintsBatched(btScalar dt);
	void UpdateVelocities(btScalar dt);
public:
	CHair& operator=(const CHair& other);
};


