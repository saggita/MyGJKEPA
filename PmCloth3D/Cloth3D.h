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
// CSpringCloth3D
//----------------------------------------------
class CSpringCloth3D
{
	friend class CCloth3D;

public:
	CSpringCloth3D() 
	{ 
		m_IndexVrx[0] = -1; 
		m_IndexVrx[1] = -1; 
		m_IndexTriangle[0] = -1;
		m_IndexTriangle[1] = -1;
		m_Index = -1;
	}

	CSpringCloth3D(int indexVrx0, int indexVrx1) 
	{ 
		m_IndexVrx[0] = indexVrx0; 
		m_IndexVrx[1] = indexVrx1;
		m_IndexTriangle[0] = -1;
		m_IndexTriangle[1] = -1;
		m_Index = -1; 
	}

	CSpringCloth3D(const CSpringCloth3D& other) 
	{
		for ( int i = 0; i < 2; i++ )
		{
			m_IndexVrx[i] = other.m_IndexVrx[i];
			m_IndexTriangle[i] = other.m_IndexTriangle[i];
		}

		m_Index = other.m_Index;
		m_RestLength = other.m_RestLength;
	}
	
	virtual ~CSpringCloth3D() { }

	CVector3D m_GradC0;
	CVector3D m_GradC1;

protected:
	int m_Index;
	int m_IndexVrx[2];
	int m_IndexTriangle[2];
	btScalar m_RestLength;

public:
	int GetVertexIndex(int i) const 
	{
		assert( 0 <= i && i <= 1 );

		return m_IndexVrx[i];
	}

	btScalar GetRestLength() const { return m_RestLength; }
	void SetRestLength(btScalar restLength) { m_RestLength = restLength; }

	int GetIndex() const { return m_Index; }
	void SetIndex(int index) { m_Index = index; }

	int GetTriangleIndex(int i) const 
	{ 
		assert( 0 <= i && i <= 1 );
		return m_IndexTriangle[i]; 
	}

	int GetTheOtherVertexIndex(int indexVert)
	{
		assert(indexVert == m_IndexVrx[0] || indexVert == m_IndexVrx[1]);

		if ( indexVert == m_IndexVrx[0] )
			return m_IndexVrx[1];
		else
			return m_IndexVrx[0];
	}

	bool operator==(const CSpringCloth3D& other)
	{
		if ( ( m_IndexVrx[0] == other.m_IndexVrx[0] && m_IndexVrx[1] == other.m_IndexVrx[1] ) ||
			 ( m_IndexVrx[0] == other.m_IndexVrx[1] && m_IndexVrx[1] == other.m_IndexVrx[0] ) )
			 return true;
		else
			return false;
	}

	CSpringCloth3D& operator=(const CSpringCloth3D& other)
	{
		for ( int i = 0; i < 2; i++ )
		{
			m_IndexVrx[i] = other.m_IndexVrx[i];
			m_IndexTriangle[i] = other.m_IndexTriangle[i];
		}

		m_Index = other.m_Index;
		m_RestLength = other.m_RestLength;

		return (*this);
	}
};

class CClothPin;

//----------------------------------------------
// CVertexCloth3D
//----------------------------------------------
class CVertexCloth3D
{
public:
	CVertexCloth3D() : m_Mass(1.0), m_InvMass(1.0), m_pPin(NULL), m_PinIndex(-1)
	{
	}

	virtual ~CVertexCloth3D() {};

	int m_Index;
	CVector3D m_Pos;
	CVector3D m_PosOld;
	CVector3D m_Vel;
	CVector3D m_dVel; // delta velocity
	btScalar m_Mass;
	btScalar m_InvMass; // = 1.0 / m_Mass. In case mass is infinite, m_InvMass is zero and m_Mass doesn't have any meaning.
					  // Currently infinite mass is not supported. CClothPin should be used to pin cloth vertex.
	CVector3D m_Force;
	CClothPin* m_pPin;
	int m_PinIndex;

	// array of indexes of stretch springs connected to this vertex 
	std::vector<int> m_StrechSpringIndexes; 

	// array of indexes of bend springs connected to this vertex 
	std::vector<int> m_BendSpringIndexes; 

	// array of indexes of triangles sharing this vertex
	std::vector<int> m_TriangleIndexes; 

public:
	int GetIndex() const { return m_Index; }
	void SetIndex(int index) { m_Index = index; }
};


//----------------------------------------------
// CClothPin
//----------------------------------------------
class CClothPin
{
public:
	CClothPin() : m_Pos(0, 0, 0), m_Vel(0, 0, 0), m_VertexIndex(-1), m_pVertex(NULL) {}
	CClothPin(CVertexCloth3D* pVertex, CVector3D pos)
	{
		if ( pVertex != NULL )
		{
			m_VertexIndex = pVertex->GetIndex();
			m_pVertex = pVertex;
		}

		m_Pos = pos;
	}
	CClothPin(const CClothPin& other)
	{
		m_VertexIndex = other.m_VertexIndex;
		m_pVertex = other.m_pVertex;
		m_Pos = other.m_Pos;		
		m_Vel = other.m_Vel;
	}
	~CClothPin() {};

private:
	int m_VertexIndex;
	CVertexCloth3D* m_pVertex;
	CVector3D m_Pos;
	CVector3D m_Vel;

public:
	CVector3D& GetPinPos() { return m_Pos; }
	const CVector3D& GetPinPos() const { return m_Pos; }
	void SetPinPos(const CVector3D& pos) { m_Pos = pos; }
	CVector3D& GetPinVelocity() { return m_Vel; }
	const CVector3D& GetPinVelocity() const { return m_Vel; }
	void SetPinVelocity(const CVector3D& vel) { m_Vel = vel; }

	CVertexCloth3D* GetPinnedVertex() { return m_pVertex; }
	const CVertexCloth3D* GetPinnedVertex() const { return m_pVertex; }
	void SetPinnedVertex(CVertexCloth3D* pVertex)
	{
		if ( pVertex != NULL )
		{
			m_VertexIndex = pVertex->GetIndex();
			m_pVertex = pVertex;
		}
	}

	CClothPin& operator=(const CClothPin& other)
	{
		m_VertexIndex = other.m_VertexIndex;
		m_pVertex = other.m_pVertex;
		m_Pos = other.m_Pos;		
		m_Vel = other.m_Vel;
		return (*this);
	}
};


//----------------------------------------------
// COLOR
//----------------------------------------------
struct COLOR
{
	COLOR() { r = g = b = 0; }
	COLOR(float R, float G, float B) { r = R; g = G; b = B; }
	float r, g, b;
};

#include "ICollidable.h"
#include "CollisionObject.h"

class CCloth3D : public ICollidable
{
public:
	CCloth3D(void);
	CCloth3D(const CCloth3D& other);
	virtual ~CCloth3D(void);

protected:
	btScalar m_dt;
	btScalar m_h; // thickness
	btScalar m_Kst; // stretch force
	btScalar m_Ksh; // shear force
	btScalar m_Kb; // bending force
	btScalar m_Kd;
	btScalar m_Epsilon; 
	btScalar m_Mu; // friction
	CVector3D m_Gravity;
	//CBVHTree* m_pBVHTree;
	bool m_bDeformable;
	COLOR m_Color;

	std::vector<CVertexCloth3D> m_VertexArray;
	std::vector<CSpringCloth3D> m_StrechSpringArray;
	std::vector<CSpringCloth3D> m_BendSpringArray;
	std::vector<CVector3D> m_NormalVecArray;
	std::vector<CTriangleCloth3D> m_TriangleArray;
	std::vector<CClothPin> m_PinArray;
	
	CCollisionObject m_CollisionObject;
	
	// for debug
	bool m_bShowBV; // toggle showing bounding volume

public:
	unsigned int m_NumIter;
	bool m_bEqualVertexMass;
	int m_NumIterForConstraintSolver;

	virtual bool Load(const char* filename);
	virtual void Initialize();
	btScalar GetKst() const { return m_Kst; };
	btScalar GetKsh() const { return m_Ksh; };
	btScalar GetKb() const { return m_Kb; };
	btScalar GetThickness() const { return m_h; }
	btScalar GetFrictionCoef() const { return m_Mu; }
	void SetKst(btScalar Kst) { m_Kst = Kst; };
	void SetKsh(btScalar Ksh) { m_Ksh = Ksh; };
	void SetKb(btScalar Kb) { m_Kb = Kb; };
	void SetThickness(btScalar h) { m_h = h; }
	void SetFrictionCoef(btScalar mu) { assert(mu >= 0 && mu <= 1.0); m_Mu = mu; }
	btScalar GetEpsilon() const { return m_Epsilon; }
	btScalar Getdt() const { return m_dt; } 
	void Setdt(btScalar dt) { m_dt = dt; }
	void SetGravity(const CVector3D& gravity);
	const CVector3D& GetGravity() const;
	bool GetShowBV() { return m_bShowBV; }
	void SetShowBV(bool bShowBV) { m_bShowBV = bShowBV; }
	void SetMassDensity(btScalar massDensity);
	void SetVertexMass(btScalar vertexMass);
	void SetTotalMass(btScalar totalMass);
	void SetNumIterForConstraintSolver(int numIter) { m_NumIterForConstraintSolver = numIter; }

	void AddPin(int vertexIndex);

	std::vector<CVertexCloth3D>& GetVertexArray() { return m_VertexArray; }
	const std::vector<CVertexCloth3D>& GetVertexArray() const { return m_VertexArray; }

	std::vector<CSpringCloth3D>& GetStrechSpringArray() { return m_StrechSpringArray; }
	const std::vector<CSpringCloth3D>& GetStrechSpringArray() const { return m_StrechSpringArray; }

	std::vector<CSpringCloth3D>& GetBendSpringArray() { return m_BendSpringArray; }
	const std::vector<CSpringCloth3D>& GetBendSpringArray() const { return m_BendSpringArray; }

	std::vector<CTriangleCloth3D>& GetTriangleArray() { return m_TriangleArray; }
	const std::vector<CTriangleCloth3D>& GetTriangleArray() const { return m_TriangleArray; }

	void FillSpringArray();

	bool IsDeformable() const { return m_bDeformable; }
	void SetDeformable(bool bDeformable) { m_bDeformable = bDeformable; }

	void Clear();
	void CalcForces();
	void IntegrateByLocalPositionContraints(btScalar dt);
	void IntegrateEuler(double dt);
	void AdvancePosition(btScalar dt);
	
	void SetColor(float r, float g, float b) { m_Color = COLOR(r, g, b); }
	virtual void Render();
	virtual CCollisionObject* GetCollisionObject() { return &m_CollisionObject; }
	virtual const CCollisionObject* GetCollisionObject() const { return &m_CollisionObject; }
	virtual void TranslateW(btScalar x, btScalar y, btScalar z);

protected:
	btScalar CalcConstraint(int indexEdge, int indexVertex, btScalar dt, CVector3D* pGradientOfConstraint = NULL);
	void EnforceEdgeConstraints(btScalar dt);

public:
	CCloth3D& operator=(const CCloth3D& other);
};


