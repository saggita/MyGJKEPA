#pragma once

#include <vector>
#include "Transform.h"
#include "../btBulletCollisionCommon.h"
#include "ConvexHeightField\ConvexHeightFieldShape.h"

struct TriangleFace
{
	int indices[3];
	btScalar planeEqn[4];
};

class CCollisionObject
{
public:
	enum CollisionObjectType { None, Point, Box, Sphere, Cone, Capsule, Cylinder, ConvexHull };

protected:
	CollisionObjectType m_CollisionObjectType; 
	CTransform m_Transform;

	CVector3D m_HalfExtent;
	btScalar m_Margin;
	btScalar m_Color[4];

	// For ConvexHull or PolyMesh
	std::vector<CVector3D> m_Vertices;
	std::vector<CVector3D> m_Normals;
	std::vector<TriangleFace> m_Faces; 

	// For visualization
	std::vector<CVector3D> m_VisualizedPoints;

	// bullet
	//btCollisionObject* m_pBulletColObj;

	// Convex HeightField
	ConvexHeightField* m_pConvexHeightField;

	bool m_bLoaded;

public:
	CCollisionObject(void);
	virtual ~CCollisionObject(void);

public:
	CollisionObjectType GetCollisionObjectType() { return m_CollisionObjectType; }
	void SetCollisionObjectType(CollisionObjectType collisionObjectType);
	
	void SetSize(btScalar x, btScalar y, btScalar z) { m_HalfExtent.Set(x/2.0f, y/2.0f, z/2.0f); }
	void SetColor(btScalar r, btScalar g, btScalar b) { m_Color[0] = r; m_Color[1] = g; m_Color[2] = b; m_Color[3] = 1.0; }

	std::vector<CVector3D>& GetVertices() { return m_Vertices; }
	const std::vector<CVector3D>& GetVertices() const { return m_Vertices; }

	std::vector<CVector3D>& GetNormals() { return m_Normals; }
	const std::vector<CVector3D>& GetNormals() const { return m_Normals; }

	std::vector<TriangleFace>& GetFaces() { return m_Faces; }
	const std::vector<TriangleFace>& GetFaces() const { return m_Faces; }

	const CTransform& GetTransform() const;
	CTransform& GetTransform();

	btScalar GetMargin() const { return m_Margin; }
	CVector3D GetSize() const { return 2.0 * m_HalfExtent; }

	CVector3D GetLocalSupportPoint(const CVector3D& dir, btScalar margin = 0) const;

	ConvexHeightField* GetConvexHFObject() { return m_pConvexHeightField; }
	const ConvexHeightField* GetConvexHFObject() const { return m_pConvexHeightField; }

	bool Load(const char* filename);

	//btCollisionObject* GetBulletObject() { return m_pBulletColObj; }

	void Render() const;

	// visualize convex heightfield
	void VisualizeHF();
};

