#ifndef __VECTOR3D_H__
#define __VECTOR3D_H__

#include <cmath>
#include "../btBulletCollisionCommon.h"
#include "Matrix44.h"
#include "Matrix33.h"

class CPoint3D;

class CVector3D
{
public:
	btScalar m_X;
	btScalar m_Y;
	btScalar m_Z;

	CVector3D() 
	{
		m_X = 0.0;
		m_Y = 0.0;
		m_Z = 0.0;
	}

	CVector3D(btScalar x, btScalar y, btScalar z) { m_X = x; m_Y = y; m_Z = z; };
	CVector3D(btScalar val) { m_X = val; m_Y = val; m_Z = val; };
	CVector3D(const CVector3D& other);
	CVector3D(const CPoint3D& other);
	CVector3D(const CPoint3D& ptBegin, const CPoint3D& ptEnd);
	CVector3D(const CVector3D& begin, const CVector3D& end);
	virtual ~CVector3D() {};

public:
	CVector3D& Normalize();
	CVector3D NormalizeOther() const;
	CVector3D Cross(const CVector3D& v) const { return CVector3D(m_Y*v.m_Z - m_Z*v.m_Y, m_Z*v.m_X - m_X*v.m_Z, m_X*v.m_Y - m_Y*v.m_X); };
	btScalar Dot(const CVector3D& v) const { return m_X * v.m_X + m_Y * v.m_Y + m_Z * v.m_Z; };
	CMatrix33 Out(const CVector3D& v) const;
	btScalar Length() const { return sqrt(m_X*m_X + m_Y*m_Y + m_Z*m_Z); };
	btScalar LengthSqr() const { return (m_X*m_X + m_Y*m_Y + m_Z*m_Z); };
	void Set(btScalar x, btScalar y, btScalar z) { m_X = x; m_Y = y; m_Z = z; };
	
	virtual void TranslateW(btScalar x, btScalar y, btScalar z);
	virtual void RotateW(const CVector3D& axis, btScalar ang);
	virtual void RotateW(CMatrix44 mat44);
	virtual void TransformW(const CMatrix44& mat44);
	
	const btScalar& operator[](unsigned int i) const
	{
		if ( i == 0 )
			return m_X;
		else if ( i == 1)
			return m_Y;
		else /*if ( i == 2 )*/
			return m_Z;
	}

	btScalar& operator[](unsigned int i) 
	{
		if ( i == 0 )
			return m_X;
		else if ( i == 1)
			return m_Y;
		else /*if ( i == 2 )*/
			return m_Z;
	}

	CVector3D& operator=(const CVector3D& other);
	CVector3D& operator=(const CPoint3D& other);
	CVector3D& operator=(btScalar val);
	bool operator!=(btScalar val) const;
	bool operator<(btScalar val) const;
	bool operator>(btScalar val) const;
	bool operator==(btScalar val) const;
	bool operator==(const CVector3D& other) const;
	CVector3D& operator-=(const CVector3D& other);
	CVector3D& operator+=(const CVector3D& other);
	CVector3D& operator*=(btScalar val);
	CVector3D operator-(const CVector3D& other) const;
	CVector3D operator+(const CVector3D& other) const;
	CVector3D operator/(btScalar val) const;
	CVector3D operator*(btScalar val) const;
	btScalar operator*(const CVector3D& other) const;
	CVector3D operator/(const CVector3D& other) const;

	friend CVector3D operator*(btScalar val, const CVector3D& other);
	friend CVector3D operator-(const CVector3D& other);

};

#endif // __VECTOR3D_H__