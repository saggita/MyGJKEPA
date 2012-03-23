#ifndef __POINT3D_H__
#define __POINT3D_H__

#include "Vector3D.h"

class CPoint3D
{
public:
	CPoint3D();
	CPoint3D(btScalar x, btScalar y, btScalar z);
	CPoint3D(const CPoint3D& other);
	CPoint3D(const CVector3D& other);
	virtual ~CPoint3D() {}

	// properties..
	btScalar m_X;
	btScalar m_Y;
	btScalar m_Z;

	virtual void Set(btScalar x, btScalar y, btScalar z);
	virtual void TranslateW(btScalar x, btScalar y, btScalar z);
	virtual void RotateW(const CVector3D& axis, btScalar ang);
	virtual void TransformW(CMatrix44 mat44);

	CPoint3D& operator=(const CPoint3D& other);
	CPoint3D operator+(const CPoint3D& other) const;
	CPoint3D operator+(const CVector3D& other) const;
	CPoint3D operator-(const CPoint3D& other) const;
	bool operator==(const CPoint3D& other);
	bool operator!=(const CPoint3D& other);

	CPoint3D operator*(btScalar val) const;
	friend CPoint3D operator*(btScalar val, const CPoint3D& other);
};

#endif // __POINT3D_H__