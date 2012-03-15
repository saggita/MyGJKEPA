#pragma once

#include <vector>
#include <list>
#include <assert.h>

#include <iostream>
#include <fstream>

#include "Point3D.h"
#include "Vector3D.h"

#include "TriangleCloth3D.h"
#include "SparseMatrix.h"
#include "CGSolver.h"

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

protected:
	int m_Index;
	int m_IndexVrx[2];
	int m_IndexTriangle[2];
	double m_RestLength;

public:
	int GetVertexIndex(int i) const 
	{
		assert( 0 <= i && i <= 1 );

		return m_IndexVrx[i];
	}

	double GetRestLength() const { return m_RestLength; }
	void SetRestLength(double restLength) { m_RestLength = restLength; }

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
	CVertexCloth3D() : m_Mass(1.0), m_Pos(0, 0, 0), m_PosOld(0, 0, 0), m_Vel(0, 0, 0), m_Force(0, 0, 0), m_pPin(NULL), m_PinIndex(-1)
	{
	}

	virtual ~CVertexCloth3D() {};

	int m_Index;
	CVector3D m_Pos;
	CVector3D m_PosOld;
	CVector3D m_Vel;
	double m_Mass;
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


class CBVHTree;

//----------------------------------------------
// COLOR
//----------------------------------------------
struct COLOR
{
	COLOR() { r = g = b = 0; }
	COLOR(float R, float G, float B) { r = R; g = G; b = B; }
	float r, g, b;
};

class ICloth3D
{
public:
	ICloth3D(void) {}
	virtual ~ICloth3D(void) {}
	
public:
	virtual void Create(const char* filename) = 0;
	virtual bool Load(const char* filename) = 0;
	virtual void Initialize() = 0;
	virtual double GetKs() const = 0;
	virtual double GetKb() const = 0;
	virtual double GetThickness() const = 0;
	virtual double GetEpsilon() const = 0;
	virtual double Getdt() const = 0;
	virtual void SetGravity(const CVector3D& gravity) = 0;
	virtual const CVector3D& GetGravity() const = 0;

	virtual std::vector<CVertexCloth3D>& GetVertexArray() = 0;
	virtual const std::vector<CVertexCloth3D>& GetVertexArray() const = 0;

	virtual std::vector<CSpringCloth3D>& GetStrechSpringArray() = 0;
	virtual const std::vector<CSpringCloth3D>& GetStrechSpringArray() const = 0;

	virtual std::vector<CSpringCloth3D>& GetBendSpringArray() = 0;
	virtual const std::vector<CSpringCloth3D>& GetBendSpringArray() const = 0;

	virtual std::vector<CTriangleCloth3D>& GetTriangleArray() = 0;
	virtual const std::vector<CTriangleCloth3D>& GetTriangleArray() const = 0;

	virtual void FillSpringArray() = 0;

	virtual bool IsDeformable() const = 0;
	virtual void SetDeformable(bool bDeformable) = 0;

	virtual void Clear() = 0;
	virtual void CalcForces(double dt) = 0;
	virtual void IntegrateEuler() = 0;
	virtual void IntegrateVerlet() = 0;
	virtual void AdvancePosition() = 0;
	virtual void StrainLimiting(double dt) = 0;
	virtual void ImplicitEulerIntegration(double dt) = 0;

	virtual void SetColor(float r, float g, float b) = 0;
	virtual void Render() = 0;
};

//class CCloth3D : public ICloth3D
class CCloth3D
{
public:
	CCloth3D(void);
	CCloth3D(std::string filepath);
	CCloth3D(const CCloth3D& other);
	virtual ~CCloth3D(void);

protected:
	double m_dt;
	double m_h; // thickness
	double m_Kst; // stretch force
	double m_Ksh; // shear force
	double m_Kb; // bending force
	double m_Kd;
	double m_Epsilon; 
	double m_Mu; // friction
	double m_MassDensity;
	CVector3D m_Gravity;
	CBVHTree* m_pBVHTree;
	bool m_bDeformable;
	COLOR m_Color;

	std::vector<CVertexCloth3D> m_VertexArray;
	std::vector<CSpringCloth3D> m_StrechSpringArray;
	std::vector<CSpringCloth3D> m_BendSpringArray;
	std::vector<CVector3D> m_NormalVecArray;
	std::vector<CTriangleCloth3D> m_TriangleArray;
	std::vector<CClothPin> m_PinArray;
	
	std::vector<CVector3D> F0;
	CColumnVector<CVector3D> f0;
	std::vector<CVector3D> V0;
	std::vector<CVector3D> temp;
	CSparseMatrix<CMatrix33> dF_dX;
	CSparseMatrix<CMatrix33> dF_dV;
	CSparseMatrix<CMatrix33> I;
	CSparseMatrix<CMatrix33> M;
	CSparseMatrix<CMatrix33> InvM;
	CSparseMatrix<CMatrix33> Pinv; // inverse of preconditioner
	/*CDiagonalMatrix<CMatrix33> I;
	CDiagonalMatrix<CMatrix33> M;
	CDiagonalMatrix<CMatrix33> InvM;*/

	CSparseMatrix<CMatrix33> A;
	std::vector<CVector3D> b;
	std::vector<CVector3D> x;

	CCGSolver<CMatrix33, CVector3D> cgSolver;

	// for debug
	bool m_bShowBV; // toggle showing bounding volume

public:
	unsigned int m_NumIter;
	bool m_bEqualVertexMass;

	virtual void Create(const char* filename);
	virtual bool Load(const char* filename);
	virtual void Initialize();
	double GetKst() const { return m_Kst; };
	double GetKsh() const { return m_Ksh; };
	double GetKb() const { return m_Kb; };
	double GetThickness() const { return m_h; }
	double GetFrictionCoef() const { return m_Mu; }
	void SetKst(double Kst) { m_Kst = Kst; };
	void SetKsh(double Ksh) { m_Ksh = Ksh; };
	void SetKb(double Kb) { m_Kb = Kb; };
	void SetThickness(double h) { m_h = h; }
	void SetFrictionCoef(double mu) { assert(mu >= 0 && mu <= 1.0); m_Mu = mu; }
	double GetEpsilon() const { return m_Epsilon; }
	double Getdt() const { return m_dt; } 
	void Setdt(double dt) { m_dt = dt; }
	void SetGravity(const CVector3D& gravity);
	const CVector3D& GetGravity() const;
	bool GetShowBV() { return m_bShowBV; }
	void SetShowBV(bool bShowBV) { m_bShowBV = bShowBV; }
	double GetMassDensity() const { return m_MassDensity; }
	void SetMassDensity(double massDensity);
	void SetVertexMass(double vertexMass);

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
	void IntegrateEuler(double dt);
	void IntegrateOnlyGravity(double dt);
	void IntegrateVerlet(double dt);
	void IntegrateRK4(double dt);
	void IntegrateBackwardEulerByBW(double dt);
	void IntegrateByPositionContraints(double dt);
	void AdvancePosition(double dt);
	void StrainLimiting(double dt);
	void EnforceEdgeConstraints(double dt);
	void EnforceVertexGradientConstraints(double dt);	
	void EnforceVertexGradientConstraintsGlobally(double dt);
	void EnforceVertexGradientConstraintsGlobally2(double dt);
	void EnforceEdgeConstraintsGlobally(double dt);
	void EnforceFaseProjectionGlobally(double dt);
	bool ImplicitEulerIntegration(double dt);

	void CalcForcesByBaraff();

	// Save obj files.
	std::string m_Filepath; // ex) c:\\temp\\fluid_animiation
	std::string m_Filename; // ex) fluidanim
	std::ofstream m_outStream;
	int m_Frame;
	void Save();
	void Save(std::string filePath);

	void SetColor(float r, float g, float b) { m_Color = COLOR(r, g, b); }
	virtual void Render();

	const CBVHTree& GetBVHTree() const { return *m_pBVHTree; }
	CBVHTree& GetBVHTree() { return *m_pBVHTree; }

	virtual void TranslateW(double x, double y, double z);

protected:
	void CalcParticleMassFromDensity();

	// For filtered implicit solver. 
	template<class T>
	static void Filter(std::vector<T>& a, const void* pCloth = NULL);	

	double CalcConstraint(int indexEdge, int indexVertex, double dt, CVector3D* pGradientOfConstraint = NULL);

public:
	CCloth3D& operator=(const CCloth3D& other);
};

class CObstacleCloth3D : public CCloth3D
{
public:
	CObstacleCloth3D(void);
	CObstacleCloth3D(std::string filepath);
	virtual ~CObstacleCloth3D(void);

public:
	virtual void Create(const char* filename);
	virtual void Initialize();
	virtual void Render();
};


