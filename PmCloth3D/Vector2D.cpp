#include "Vector2D.h"
#include "Point2D.h"

CVector2D::CVector2D(const CVector2D& other) 
{ 
	m_X = other.m_X; 
	m_Y = other.m_Y; 
}

CVector2D::CVector2D(const CPoint2D& other) 
{ 
	m_X = other.m_X; 
	m_Y = other.m_Y; 
}

CVector2D::CVector2D(const CPoint2D& ptBegin, const CPoint2D& ptEnd) 
{ 
	m_X = ptEnd.m_X - ptBegin.m_X; 
	m_Y = ptEnd.m_Y - ptBegin.m_Y; 
};  

CVector2D::CVector2D(const CVector2D& begin, const CVector2D& end)
{
	m_X = end.m_X - begin.m_X; 
	m_Y = end.m_Y - begin.m_Y; 
}

CVector2D& CVector2D::Normalize()
{
	btScalar d = sqrt(m_X * m_X + m_Y * m_Y);

	if ( d == 0 )
		return *this;

	m_X = m_X / d;
	m_Y = m_Y / d;

	return *this;
}

//void CVector2D::TranslateW(btScalar x, btScalar y)
//{
//	// No effect on 3D vector
//}

//void CVector2D::RotateW(const CVector2D& axis, btScalar ang)
//{
//	CMatrix44 mat44;
//	mat44.SetIdentity();
//	mat44.SetRotation(axis, ang);
//
//	RotateW(mat44);
//}
//
//void CVector2D::RotateW(CMatrix44 mat44)
//{
//	*this = mat44 * (*this);
//}
//
//void CVector2D::TransformW(const CMatrix44& mat44)
//{
//
//}

CVector2D CVector2D::operator-(const CVector2D& other) const
{ 
	return CVector2D(m_X - other.m_X, m_Y - other.m_Y); 
}

CVector2D CVector2D::operator+(const CVector2D& other) const
{ 
	return CVector2D(m_X + other.m_X, m_Y + other.m_Y); 
}

CVector2D CVector2D::operator/(btScalar val) const
{ 
	if ( val != 0 )
		return CVector2D(m_X / val, m_Y / val); 
	else
		return CVector2D(0.0f, 0.0f);
}

CVector2D& CVector2D::operator=(const CVector2D& other) 
{ 
	m_X = other.m_X; 
	m_Y = other.m_Y; 
	
	return *this; 
}

CVector2D& CVector2D::operator=(const CPoint2D& other) 
{ 
	m_X = other.m_X; 
	m_Y = other.m_Y; 
	
	return *this; 
}

CVector2D& CVector2D::operator-=(const CVector2D& other) 
{ 
	m_X -= other.m_X; 
	m_Y -= other.m_Y; 
	
	return *this; 
}

CVector2D& CVector2D::operator+=(const CVector2D& other) 
{ 
	m_X += other.m_X; 
	m_Y += other.m_Y; 
	
	return *this; 
}

CVector2D& CVector2D::operator*=(btScalar val)
{
	m_X *= val; 
	m_Y *= val; 
	
	return *this; 
}

CVector2D CVector2D::operator*(btScalar val) const
{
	CVector2D vec(*this);

	vec.m_X *= val;
	vec.m_Y *= val;

	return vec;
}

CVector2D operator*(btScalar val, const CVector2D& other)
{
	return other * val;
}


CVector2D operator-(const CVector2D& other)
{
	CVector2D vec(other);

	vec.m_X = -vec.m_X;
	vec.m_Y = -vec.m_Y;

	return vec;
}
