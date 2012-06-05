#include "mathUtil.h"
#include "Point3D.h"
#include "Vector3D.h"

/*
#define PI_F 3.1415926535898f
#define EPSILON 1e-8
#define EPSILON1 0.00001f

inline bool IsZero(float x) { return ( ( x < EPSILON ) && ( x > -EPSILON ) ? true : false ); }
inline bool IsEqual(float a, float b) { return ( ( ( a - b ) < EPSILON1 ) && ( ( a - b ) > -EPSILON1 ) ? true : false ); }
*/

CPoint3D::CPoint3D()
{
	m_X = 0.0f; 
	m_Y = 0.0f; 
	m_Z = 0.0f;
}

CPoint3D::CPoint3D(float x, float y, float z)
{
	m_X = x; 
	m_Y = y; 
	m_Z = z;
}

CPoint3D::CPoint3D(const CPoint3D& other)
{
	m_X = other.m_X; 
	m_Y = other.m_Y; 
	m_Z = other.m_Z;
}

CPoint3D::CPoint3D(const CVector3D& other)
{
	m_X = other.m_X; 
	m_Y = other.m_Y; 
	m_Z = other.m_Z;
}

void CPoint3D::Set(float x, float y, float z)
{
	m_X = x;
	m_Y = y;
	m_Z = z;
}

void CPoint3D::TranslateW(float x, float y, float z)
{
	m_X = m_X + x;
	m_Y = m_Y + y;
	m_Z = m_Z + z;
}

void CPoint3D::RotateW(const CVector3D& axis, float ang)
{
	CMatrix44 mat44;
	mat44.SetIdentity();
	mat44.SetRotation(axis, ang);

	TransformW(mat44);
}

void CPoint3D::TransformW(CMatrix44 mat44)
{
	*this = mat44 * (*this);
}

CPoint3D& CPoint3D::operator=(const CPoint3D& other)
{
	m_X = other.m_X; 
	m_Y = other.m_Y; 
	m_Z = other.m_Z;

	return *this;
}

CPoint3D CPoint3D::operator+(const CPoint3D& other) const
{ 
	CPoint3D pnt(m_X + other.m_X, m_Y + other.m_Y, m_Z + other.m_Z); 
	
	return pnt;
}

CPoint3D CPoint3D::operator+(const CVector3D& other) const
{ 
	CPoint3D pnt(m_X + other.m_X, m_Y + other.m_Y, m_Z + other.m_Z); 
	
	return pnt;
}

CPoint3D CPoint3D::operator-(const CPoint3D& other) const
{ 
	CPoint3D pnt(m_X - other.m_X, m_Y - other.m_Y, m_Z - other.m_Z); 
	
	return pnt;
}

bool CPoint3D::operator==(const CPoint3D& other)
{
	return ( IsEqual(m_X, other.m_X) && IsEqual(m_Y, other.m_Y) && IsEqual(m_Z, other.m_Z) );
}

bool CPoint3D::operator!=(const CPoint3D& other)
{
	return !( IsEqual(m_X, other.m_X) && IsEqual(m_Y, other.m_Y) && IsEqual(m_Z, other.m_Z) );
}

CPoint3D CPoint3D::operator*(float val) const
{
	CPoint3D vec(*this);

	vec.m_X *= val;
	vec.m_Y *= val;
	vec.m_Z *= val;

	return vec;
}

CPoint3D operator*(float val, const CPoint3D& other)
{
	return other * val;
}


