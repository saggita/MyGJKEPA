#ifndef __VECTOR2D_H__
#define __VECTOR2D_H__

#include <cmath>
#include "../btBulletCollisionCommon.h"
#include "Matrix44.h"

class CPoint2D;

class CVector2D
{
public:
	btScalar m_X;
	btScalar m_Y;

	CVector2D() 
	{
		m_X = 0;
		m_Y = 0;
	}

	CVector2D(btScalar x, btScalar y) { m_X = x; m_Y = y; };
	CVector2D(const CVector2D& other);
	CVector2D(const CPoint2D& other);
	CVector2D(const CPoint2D& ptBegin, const CPoint2D& ptEnd);
	CVector2D(const CVector2D& begin, const CVector2D& end);
	virtual ~CVector2D() {};

public:
	CVector2D& Normalize();
	//CVector2D Cross(const CVector2D& v) const { return CVector2D(m_Y*v.m_Z - m_Z*v.m_Y, m_Z*v.m_X - m_X*v.m_Z, m_X*v.m_Y - m_Y*v.m_X); };
	btScalar Dot(const CVector2D& v) const { return m_X * v.m_X + m_Y * v.m_Y; };
	btScalar Length() const { return sqrt(m_X*m_X + m_Y*m_Y); };
	void Set(btScalar x, btScalar y) { m_X = x; m_Y = y; };
	
	/*virtual void TranslateW(btScalar x, btScalar y, btScalar z);
	virtual void RotateW(const CVector2D& axis, btScalar ang);
	virtual void RotateW(CMatrix44 mat44);
	virtual void TransformW(const CMatrix44& mat44);*/
	
	CVector2D& operator=(const CVector2D& other);
	CVector2D& operator=(const CPoint2D& other);
	CVector2D& operator-=(const CVector2D& other);
	CVector2D& operator+=(const CVector2D& other);
	CVector2D& operator*=(btScalar val);
	CVector2D operator-(const CVector2D& other) const;
	CVector2D operator+(const CVector2D& other) const;
	CVector2D operator/(btScalar val) const;
	CVector2D operator*(btScalar val) const;

	friend CVector2D operator*(btScalar val, const CVector2D& other);
	friend CVector2D operator-(const CVector2D& other);

};

#endif // __VECTOR2D_H__