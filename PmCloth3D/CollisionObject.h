#pragma once

#include "Transform.h"

//#include "btBulletCollisionCommon.h"

class CCollisionObject
{
public:
	CCollisionObject(void);
	virtual ~CCollisionObject(void);

	enum CollisionObjectType { None, Point, Box, Sphere, Cone, Capsule, Cylinder, ConvexHull, PolyMesh };


protected:
	CollisionObjectType m_CollisionObjectType; 
	CTransform m_Transform;

	CVector3D m_HalfExtent;
	double m_Margin;
	float m_Color[4];

	// bullet
	//btCollisionObject* m_pBulletColObj;

public:
	CollisionObjectType GetCollisionObjectType() { return m_CollisionObjectType; }
	void SetCollisionObjectType(CollisionObjectType collisionObjectType) { m_CollisionObjectType = collisionObjectType; }
	
	void SetSize(double x, double y, double z) { m_HalfExtent.Set(x/2.0, y/2.0, z/2.0); }
	void SetColor(float r, float g, float b) { m_Color[0] = r; m_Color[1] = g; m_Color[2] = b; m_Color[3] = 1.0; }

	const CTransform& GetTransform() const;
	CTransform& GetTransform();

	double GetMargin() const { return m_Margin; }
	CVector3D GetSize() const { return 2.0 * m_HalfExtent; }

	CVector3D GetLocalSupportPoint(const CVector3D& dir, double margin = 0) const;

	//btCollisionObject* GetBulletObject() { return m_pBulletColObj; }

	void Render() const;
};

