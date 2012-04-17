#include <assert.h>
#include "Vector3D.h"
#include "Point3D.h"

CVector3D::CVector3D(const CVector3D& other) 
{ 
	m_X = other.m_X; 
	m_Y = other.m_Y; 
	m_Z = other.m_Z; 
}

CVector3D::CVector3D(const CPoint3D& other) 
{ 
	m_X = other.m_X; 
	m_Y = other.m_Y; 
	m_Z = other.m_Z; 
}

CVector3D::CVector3D(const CPoint3D& ptBegin, const CPoint3D& ptEnd) 
{ 
	m_X = ptEnd.m_X - ptBegin.m_X; 
	m_Y = ptEnd.m_Y - ptBegin.m_Y; 
	m_Z = ptEnd.m_Z - ptBegin.m_Z; 
};  

CVector3D::CVector3D(const CVector3D& begin, const CVector3D& end)
{
	m_X = end.m_X - begin.m_X; 
	m_Y = end.m_Y - begin.m_Y; 
	m_Z = end.m_Z - begin.m_Z; 
}

CVector3D& CVector3D::Normalize()
{
	btScalar d = sqrt(m_X * m_X + m_Y * m_Y + m_Z * m_Z);

	if ( d == 0 )
		return *this;

	m_X = m_X / d;
	m_Y = m_Y / d;
	m_Z = m_Z / d;

	return *this;
}

CVector3D CVector3D::NormalizeOther() const
{
	CVector3D n(*this);
	return n.Normalize();
}

CMatrix33 CVector3D::Out(const CVector3D& v) const
{
	CMatrix33 m;

	m.e[0][0] = m_X * v.m_X;	m.e[0][1] = m_X * v.m_Y;	m.e[0][2] = m_X * v.m_Z;
	m.e[1][0] = m_Y * v.m_X;	m.e[1][1] = m_Y * v.m_Y;	m.e[1][2] = m_Y * v.m_Z;
	m.e[2][0] = m_Z * v.m_X;	m.e[2][1] = m_Z * v.m_Y;	m.e[2][2] = m_Z * v.m_Z;
	
	return m;
}

void CVector3D::TranslateW(btScalar x, btScalar y, btScalar z)
{
	// No effect on 3D vector
}

void CVector3D::RotateW(const CVector3D& axis, btScalar ang)
{
	CMatrix44 mat44;
	mat44.SetIdentity();
	mat44.SetRotation(axis, ang);

	RotateW(mat44);
}

void CVector3D::RotateW(CMatrix44 mat44)
{
	*this = mat44 * (*this);
}

void CVector3D::TransformW(const CMatrix44& mat44)
{

}

CVector3D CVector3D::operator-(const CVector3D& other) const
{ 
	return CVector3D(m_X - other.m_X, m_Y - other.m_Y, m_Z - other.m_Z); 
}

CVector3D CVector3D::operator+(const CVector3D& other) const
{ 
	return CVector3D(m_X + other.m_X, m_Y + other.m_Y, m_Z + other.m_Z); 
}

CVector3D CVector3D::operator/(btScalar val) const
{ 
	if ( val != 0.0f )
		return CVector3D(m_X / val, m_Y / val, m_Z / val); 
	else
		return CVector3D(0.0f, 0.0f, 0.0f);
}

CVector3D& CVector3D::operator=(const CVector3D& other) 
{ 
	m_X = other.m_X; 
	m_Y = other.m_Y; 
	m_Z = other.m_Z; 
	
	return *this; 
}

CVector3D& CVector3D::operator=(const CPoint3D& other) 
{ 
	m_X = other.m_X; 
	m_Y = other.m_Y; 
	m_Z = other.m_Z; 
	
	return *this; 
}

CVector3D& CVector3D::operator=(btScalar val)
{
	m_X = val; 
	m_Y = val; 
	m_Z = val; 
	
	return *this; 
}

bool CVector3D::operator<(btScalar val) const
{
	return (LengthSqr() < val*val);
}

bool CVector3D::operator>(btScalar val) const
{
	return (LengthSqr() > val*val);
}

bool CVector3D::operator!=(btScalar val) const
{
	return (m_X != val || m_Y != val || m_Z != val );
}

bool CVector3D::operator==(btScalar val) const
{
	return (m_X == val && m_Y == val && m_Z == val );
}

bool CVector3D::operator==(const CVector3D& other) const
{
	return (m_X == other.m_X && m_Y == other.m_Y && m_Z == other.m_Z );
}

bool CVector3D::operator!=(const CVector3D& other) const
{
	return (m_X != other.m_X || m_Y != other.m_Y || m_Z != other.m_Z );
}

CVector3D& CVector3D::operator-=(const CVector3D& other) 
{ 
	m_X -= other.m_X; 
	m_Y -= other.m_Y; 
	m_Z -= other.m_Z; 
	
	return *this; 
}

CVector3D& CVector3D::operator+=(const CVector3D& other) 
{ 
	m_X += other.m_X; 
	m_Y += other.m_Y; 
	m_Z += other.m_Z; 
	
	return *this; 
}

CVector3D& CVector3D::operator*=(btScalar val)
{
	m_X *= val; 
	m_Y *= val; 
	m_Z *= val; 
	
	return *this; 
}

CVector3D CVector3D::operator*(btScalar val) const
{
	CVector3D vec(*this);

	vec.m_X *= val;
	vec.m_Y *= val;
	vec.m_Z *= val;

	return vec;
}

btScalar CVector3D::operator*(const CVector3D& other) const
{
	return Dot(other);
}

CVector3D CVector3D::operator/(const CVector3D& other) const
{
	CVector3D vec;

	btScalar e = 1e-10f;

	btScalar x = 0;
	btScalar y = 0;
	btScalar z = 0;

	if ( 0 <= other.m_X && other.m_X <= e )
		x = other.m_X + e;
	else if ( -e <= other.m_X && other.m_X <= 0 )
		x = other.m_X - e;
	else
		x = other.m_X;

	if ( 0 <= other.m_Y && other.m_Y <= e )
		y = other.m_Y + e;
	else if ( -e <= other.m_Y && other.m_Y <= 0 )
		y = other.m_Y - e;
	else
		y = other.m_Y;

	if ( 0 <= other.m_Z && other.m_Z <= e )
		z = other.m_Z + e;
	else if ( -e <= other.m_Z && other.m_Z <= 0 )
		z = other.m_Z - e;
	else
		z = other.m_Z;

	vec.m_X = m_X / x;
	vec.m_Y = m_Y / y;
	vec.m_Z = m_Z / z;

	return vec;
}

CVector3D operator*(btScalar val, const CVector3D& other)
{
	return other * val;
}


CVector3D operator-(const CVector3D& other)
{
	CVector3D vec(other);

	vec.m_X = -vec.m_X;
	vec.m_Y = -vec.m_Y;
	vec.m_Z = -vec.m_Z;

	return vec;
}
