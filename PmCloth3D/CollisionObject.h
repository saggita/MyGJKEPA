#pragma once

#include <vector>
#include "Transform.h"
#include "../btBulletCollisionCommon.h"
#include "ConvexHeightField\ConvexHeightFieldShape.h"
#include "ConvexHeightField\ChNarrowphase.h"
#include "ICollidable.h"

//------------------
class CTriangleFace
//------------------
{
public:
	CTriangleFace();
	CTriangleFace(const CTriangleFace& other);
	virtual ~CTriangleFace();

	bool m_bFlag; 

protected:
	int m_Index;
	int m_IndexVrx[3];
	int m_IndexEdge[3];
	float m_PlaneEqn[4];
	
public:
	int GetVertexIndex(int i) const 
	{
		assert( 0 <= i && i < 3 );
		return m_IndexVrx[i];
	}

	void SetVertexIndex(int i, int vertexIndex)
	{
		assert( 0 <= i && i < 3 );
		 m_IndexVrx[i] = vertexIndex;
	}

	int GetEdgeIndex(int i) const 
	{
		assert( 0 <= i && i < 3 );
		return m_IndexEdge[i];
	}

	void SetEdgeIndex(int i, int edgeIndex) 
	{
		assert( 0 <= i && i < 3 );
		m_IndexEdge[i] = edgeIndex;
	}

	int GetIndex() const { return m_Index; }
	void SetIndex(int index) { m_Index = index; }

	float* PlaneEquation() { return m_PlaneEqn; }
	const float* PlaneEquation() const { return m_PlaneEqn; }

	CVector3D GetNormal() const { return CVector3D(m_PlaneEqn[0], m_PlaneEqn[1], m_PlaneEqn[2]); }
	
	CTriangleFace& operator=(const CTriangleFace& other);
};


//----------
class CEdge
//----------
{
public:
	CEdge() : m_bFlag(false)
	{ 
		m_IndexVrx[0] = -1; 
		m_IndexVrx[1] = -1; 
		m_IndexTriangle[0] = -1;
		m_IndexTriangle[1] = -1;
		m_Index = -1;
	}

	CEdge(int indexVrx0, int indexVrx1) : m_bFlag(false)
	{ 
		m_IndexVrx[0] = indexVrx0; 
		m_IndexVrx[1] = indexVrx1;
		m_IndexTriangle[0] = -1;
		m_IndexTriangle[1] = -1;
		m_Index = -1; 
	}

	CEdge(const CEdge& other) 
	{
		for ( int i = 0; i < 2; i++ )
		{
			m_IndexVrx[i] = other.m_IndexVrx[i];
			m_IndexTriangle[i] = other.m_IndexTriangle[i];
		}

		m_bFlag = other.m_bFlag;

		m_Index = other.m_Index;
	}
	
	virtual ~CEdge() { }

	bool m_bFlag; 
	
protected:
	int m_Index;
	int m_IndexVrx[2];
	int m_IndexTriangle[2];

public:
	int GetVertexIndex(int i) const 
	{
		assert( 0 <= i && i <= 1 );

		return m_IndexVrx[i];
	}
	
	int GetIndex() const { return m_Index; }
	void SetIndex(int index) { m_Index = index; }

	int GetTriangleIndex(int i) const 
	{ 
		assert( 0 <= i && i <= 1 );
		return m_IndexTriangle[i]; 
	}

	void SetTriangleIndex(int i, int indexTriangle) 
	{
		assert( 0 <= i && i <= 1 );
		m_IndexTriangle[i] = indexTriangle;
	}

	int GetTheOtherVertexIndex(int indexVert)
	{
		assert(indexVert == m_IndexVrx[0] || indexVert == m_IndexVrx[1]);

		if ( indexVert == m_IndexVrx[0] )
			return m_IndexVrx[1];
		else
			return m_IndexVrx[0];
	}

	bool operator==(const CEdge& other)
	{
		if ( ( m_IndexVrx[0] == other.m_IndexVrx[0] && m_IndexVrx[1] == other.m_IndexVrx[1] ) ||
			 ( m_IndexVrx[0] == other.m_IndexVrx[1] && m_IndexVrx[1] == other.m_IndexVrx[0] ) )
			 return true;
		else
			return false;
	}

	CEdge& operator=(const CEdge& other)
	{
		for ( int i = 0; i < 2; i++ )
		{
			m_IndexVrx[i] = other.m_IndexVrx[i];
			m_IndexTriangle[i] = other.m_IndexTriangle[i];
		}

		m_bFlag = other.m_bFlag;
		m_Index = other.m_Index;

		return (*this);
	}
};

//-------------------------------------------
class CCollisionObject : public ICollidable
//-------------------------------------------
{
public:
	enum CollisionObjectType { None, Point, LineSegment, Box, Sphere, Cone, Capsule, Cylinder, ConvexHull };

protected:
	CollisionObjectType m_CollisionObjectType; 

	// transforms local to world. 
	CTransform m_Transform;

	CVector3D m_HalfExtent;
	
	float m_Color[4];

	// For ConvexHull
	std::vector<CVector3D> m_Vertices;	
	std::vector<CVector3D> m_Normals;
	std::vector<CTriangleFace> m_Faces;
	std::vector<CEdge> m_Edges;

	// For visualization
	std::vector<CVector3D> m_VisualizedPoints;

	// bullet
	//btCollisionObject* m_pBulletColObj;

	// Convex HeightField
	ConvexHeightField* m_pConvexHeightField;

	bool m_bLoaded;

	//--------
	// OpenCL
	//--------
	Device* m_ddcl; 
	Device* m_ddhost;	
	ShapeDataType m_ShapeBuffer;
	ChNarrowphase<TYPE_CL>::Data* m_Data;
	HostBuffer<RigidBodyBase::Body>* m_pBufRBodiesCPU;
	Buffer<RigidBodyBase::Body>* m_pBufRBodiesGPU;

public:
	CCollisionObject();
	CCollisionObject(const CCollisionObject& other);
	CCollisionObject(Device* ddcl, Device* ddhost);
	virtual ~CCollisionObject(void);

public:
	bool Create();

	CollisionObjectType GetCollisionObjectType() { return m_CollisionObjectType; }
	void SetCollisionObjectType(CollisionObjectType collisionObjectType);
	
	void SetSize(float x, float y, float z) { m_HalfExtent.Set(x/2.0f, y/2.0f, z/2.0f); }
	void SetColor(float r, float g, float b) { m_Color[0] = r; m_Color[1] = g; m_Color[2] = b; m_Color[3] = 1.0; }

	std::vector<CVector3D>& GetVertices() { return m_Vertices; }
	const std::vector<CVector3D>& GetVertices() const { return m_Vertices; }

	std::vector<CVector3D>& GetNormals() { return m_Normals; }
	const std::vector<CVector3D>& GetNormals() const { return m_Normals; }

	std::vector<CTriangleFace>& GetFaces() { return m_Faces; }
	const std::vector<CTriangleFace>& GetFaces() const { return m_Faces; }

	std::vector<CEdge>& GetEdges() { return m_Edges; }
	const std::vector<CEdge>& GetEdges() const { return m_Edges; }

	const CTransform& GetTransform() const;
	CTransform& GetTransform();

	
	CVector3D GetSize() const { return 2.0 * m_HalfExtent; }

	CVector3D GetLocalSupportPoint(const CVector3D& dir, float margin = 0) const;

	ConvexHeightField* GetConvexHFObject() { return m_pConvexHeightField; }
	const ConvexHeightField* GetConvexHFObject() const { return m_pConvexHeightField; }

	bool Load(const char* filename);

	virtual CCollisionObject* GetCollisionObject() { return this; }
	virtual const CCollisionObject* GetCollisionObject() const { return this; }

	//btCollisionObject* GetBulletObject() { return m_pBulletColObj; }

	void Render(bool bWireframe = false) const;

	// visualize convex heightfield
	void VisualizeHF();

	CCollisionObject& operator=(const CCollisionObject& other);
};

