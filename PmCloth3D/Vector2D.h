#ifndef __VECTOR2D_H__
#define __VECTOR2D_H__

#include <cmath>
#include "../btBulletCollisionCommon.h"
#include "Matrix44.h"

class CPoint2D;

class CVector2D
{
public:
	float m_X;
	float m_Y;

	CVector2D() 
	{
		m_X = 0;
		m_Y = 0;
	}

	CVector2D(float x, float y) { m_X = x; m_Y = y; };
	CVector2D(const CVector2D& other);
	CVector2D(const CPoint2D& other);
	CVector2D(const CPoint2D& ptBegin, const CPoint2D& ptEnd);
	CVector2D(const CVector2D& begin, const CVector2D& end);
	virtual ~CVector2D() {};

public:
	CVector2D& Normalize();
	//CVector2D Cross(const CVector2D& v) const { return CVector2D(m_Y*v.m_Z - m_Z*v.m_Y, m_Z*v.m_X - m_X*v.m_Z, m_X*v.m_Y - m_Y*v.m_X); };
	float Dot(const CVector2D& v) const { return m_X * v.m_X + m_Y * v.m_Y; };
	float Length() const { return sqrt(m_X*m_X + m_Y*m_Y); };
	void Set(float x, float y) { m_X = x; m_Y = y; };
	
	/*virtual void TranslateW(float x, float y, float z);
	virtual void RotateW(const CVector2D& axis, float ang);
	virtual void RotateW(CMatrix44 mat44);
	virtual void TransformW(const CMatrix44& mat44);*/
	
	CVector2D& operator=(const CVector2D& other);
	CVector2D& operator=(const CPoint2D& other);
	CVector2D& operator-=(const CVector2D& other);
	CVector2D& operator+=(const CVector2D& other);
	CVector2D& operator*=(float val);
	CVector2D operator-(const CVector2D& other) const;
	CVector2D operator+(const CVector2D& other) const;
	CVector2D operator/(float val) const;
	CVector2D operator*(float val) const;

	friend CVector2D operator*(float val, const CVector2D& other);
	friend CVector2D operator-(const CVector2D& other);

};

#endif // __VECTOR2D_H__