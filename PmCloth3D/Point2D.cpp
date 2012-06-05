#include "mathUtil.h"
#include "Point2D.h"
#include "Vector3D.h"

/*
#define PI_F 3.1415926535898f
#define EPSILON 1e-8
#define EPSILON1 0.00001f

inline bool IsZero(float x) { return ( ( x < EPSILON ) && ( x > -EPSILON ) ? true : false ); }
inline bool IsEqual(float a, float b) { return ( ( ( a - b ) < EPSILON1 ) && ( ( a - b ) > -EPSILON1 ) ? true : false ); }
*/

CPoint2D::CPoint2D()
{
	m_X = 0; 
	m_Y = 0; 
}

CPoint2D::CPoint2D(float x, float y)
{
	m_X = x; 
	m_Y = y; 
}

CPoint2D::CPoint2D(const CPoint2D& other)
{
	m_X = other.m_X; 
	m_Y = other.m_Y; 
}

CPoint2D::CPoint2D(const CVector2D& other)
{
	m_X = other.m_X; 
	m_Y = other.m_Y;
}

void CPoint2D::Set(float x, float y)
{
	m_X = x;
	m_Y = y;
}

void CPoint2D::TranslateW(float x, float y)
{
	m_X = m_X + x;
	m_Y = m_Y + y;
}

//void CPoint2D::RotateW(const CVector3D& axis, float ang)
//{
//	CMatrix44 mat44;
//	mat44.SetIdentity();
//	mat44.SetRotation(axis, ang);
//
//	TransformW(mat44);
//}
//
//void CPoint2D::TransformW(CMatrix44 mat44)
//{
//	*this = mat44 * (*this);
//}

CPoint2D& CPoint2D::operator=(const CPoint2D& other)
{
	m_X = other.m_X; 
	m_Y = other.m_Y; 

	return *this;
}

CPoint2D CPoint2D::operator+(const CPoint2D& other) const
{ 
	CPoint2D pnt(m_X + other.m_X, m_Y + other.m_Y); 
	
	return pnt;
}

CPoint2D CPoint2D::operator-(const CPoint2D& other) const
{ 
	CPoint2D pnt(m_X - other.m_X, m_Y - other.m_Y); 
	
	return pnt;
}

//CPoint2D CPoint2D::operator+(const CVector3D& other) const
//{ 
//	CPoint2D pnt(m_X + other.m_X, m_Y + other.m_Y, m_Z + other.m_Z); 
//	
//	return pnt;
//}

bool CPoint2D::operator==(const CPoint2D& other)
{
	return ( IsEqual(m_X, other.m_X) && IsEqual(m_Y, other.m_Y));
}

bool CPoint2D::operator!=(const CPoint2D& other)
{
	return !( IsEqual(m_X, other.m_X) && IsEqual(m_Y, other.m_Y));
}

CPoint2D CPoint2D::operator*(float val) const
{
	CPoint2D vec(*this);

	vec.m_X *= val;
	vec.m_Y *= val;

	return vec;
}

CPoint2D operator*(float val, const CPoint2D& other)
{
	return other * val;
}


