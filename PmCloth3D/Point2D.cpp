#include "mathUtil.h"
#include "Point2D.h"
#include "Vector3D.h"

/*
#define PI_F 3.1415926535898f
#define EPSILON 1e-8
#define EPSILON1 0.00001f

inline bool IsZero(double x) { return ( ( x < EPSILON ) && ( x > -EPSILON ) ? true : false ); }
inline bool IsEqual(double a, double b) { return ( ( ( a - b ) < EPSILON1 ) && ( ( a - b ) > -EPSILON1 ) ? true : false ); }
*/

CPoint2D::CPoint2D()
{
	m_X = 0; 
	m_Y = 0; 
}

CPoint2D::CPoint2D(double x, double y)
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

void CPoint2D::Set(double x, double y)
{
	m_X = x;
	m_Y = y;
}

void CPoint2D::TranslateW(double x, double y)
{
	m_X = m_X + x;
	m_Y = m_Y + y;
}

//void CPoint2D::RotateW(const CVector3D& axis, double ang)
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

CPoint2D CPoint2D::operator*(double val) const
{
	CPoint2D vec(*this);

	vec.m_X *= val;
	vec.m_Y *= val;

	return vec;
}

CPoint2D operator*(double val, const CPoint2D& other)
{
	return other * val;
}


