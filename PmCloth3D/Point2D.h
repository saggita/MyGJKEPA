#ifndef __POINT2D_H__
#define __POINT2D_H__

#include "../btBulletCollisionCommon.h"
#include "Vector2D.h"

class CPoint2D
{
public:
	CPoint2D();
	CPoint2D(btScalar x, btScalar y);
	CPoint2D(const CPoint2D& other);
	CPoint2D(const CVector2D& other);
	virtual ~CPoint2D() {}

	// properties..
	btScalar m_X;
	btScalar m_Y;

	virtual void Set(btScalar x, btScalar y);
	virtual void TranslateW(btScalar x, btScalar y);
	//virtual void RotateW(const CVector3D& axis, btScalar ang);
	//virtual void TransformW(CMatrix44 mat44);

	CPoint2D& operator=(const CPoint2D& other);
	CPoint2D operator+(const CPoint2D& other) const;
	CPoint2D operator-(const CPoint2D& other) const;
	//CPoint2D operator+(const CVector3D& other) const;
	bool operator==(const CPoint2D& other);
	bool operator!=(const CPoint2D& other);

	CPoint2D operator*(btScalar val) const;
	friend CPoint2D operator*(btScalar val, const CPoint2D& other);
};

#endif // __POINT2D_H__