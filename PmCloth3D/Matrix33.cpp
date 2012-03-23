#include <cassert>

#include "Matrix33.h"
#include "Vector3D.h"

const CMatrix33 CMatrix33::IDENTITY = CMatrix33(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
const CMatrix33 CMatrix33::ZERO = CMatrix33(0.0);

#define vsin(x) ((1.0f) - cos(x)) 

CMatrix33::CMatrix33(void)
{
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			e[i][j] = 0;
}

CMatrix33::CMatrix33(const CMatrix33& other)
{
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			e[i][j] = other.e[i][j];
}

CMatrix33::CMatrix33(btScalar e00, btScalar e01, btScalar e02, btScalar e10, btScalar e11, btScalar e12, btScalar e20, btScalar e21, btScalar e22)
{
	e[0][0] = e00;
	e[0][1] = e01;
	e[0][2] = e02;
	
	e[1][0] = e10;
	e[1][1] = e11;
	e[1][2] = e12;

	e[2][0] = e20;
	e[2][1] = e21;
	e[2][2] = e22;
}

CMatrix33::CMatrix33(btScalar a)
{
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			e[i][j] = a;
}

CMatrix33::~CMatrix33(void)
{
}

void CMatrix33::SetIdentity()
{
	e[0][0] = 1.0;
	e[0][1] = 0.0;
	e[0][2] = 0.0;
	
	e[1][0] = 0.0;
	e[1][1] = 1.0;
	e[1][2] = 0.0;

	e[2][0] = 0.0;
	e[2][1] = 0.0;
	e[2][2] = 1.0;
}

btScalar CMatrix33::GetElement(int i, int j) const
{
	assert(0 <= i && i < 3);
	assert(0 <= i && j < 3);

	return e[i][j];
}

void CMatrix33::SetElement(int i, int j, btScalar val)
{
	assert(0 <= i && i < 3);
	assert(0 <= i && j < 3);

	e[i][j] = val;
}

void CMatrix33::Set(btScalar e00, btScalar e01, btScalar e02, btScalar e10, btScalar e11, btScalar e12, btScalar e20, btScalar e21, btScalar e22)
{
	e[0][0] = e00;
	e[0][1] = e01;
	e[0][2] = e02;
	
	e[1][0] = e10;
	e[1][1] = e11;
	e[1][2] = e12;

	e[2][0] = e20;
	e[2][1] = e21;
	e[2][2] = e22;
}

void CMatrix33::SetRotation(const CVector3D& axis, btScalar ang)
{
	btScalar nx = axis.m_X; 
	btScalar ny = axis.m_Y;
	btScalar nz = axis.m_Z;

	e[0][0] = nx*nx*vsin(ang) + cos(ang);
	e[0][1] = nx*ny*vsin(ang) - nz*sin(ang);
	e[0][2] = nx*nz*vsin(ang) + ny*sin(ang);
	
	e[1][0] = nx*ny*vsin(ang) + nz*sin(ang);
	e[1][1] = ny*ny*vsin(ang) + cos(ang);
	e[1][2] = ny*nz*vsin(ang) - nx*sin(ang);

	e[2][0] = nx*nz*vsin(ang) - ny*sin(ang);
	e[2][1] = ny*nz*vsin(ang) + nx*sin(ang);
	e[2][2] = nz*nz*vsin(ang) + cos(ang);
}

void CMatrix33::Inverse()
{
	btScalar det = e[0][0] * ( e[2][2] * e[1][1] - e[2][1] * e[1][2] ) - 
				 e[1][0] * ( e[2][2] * e[0][1] - e[2][1] * e[0][2] ) +
				 e[2][0] * ( e[1][2] * e[0][1] - e[1][1] * e[0][2] );

	e[0][0] = e[2][2] * e[1][1] - e[2][1] * e[1][2];
	e[0][1] = -e[2][2] * e[0][1] - e[2][1] * e[0][2];
	e[0][2] = e[1][2] * e[0][1] - e[1][1] * e[0][2];

	e[1][0] = -e[2][2] * e[1][0] - e[2][0] * e[1][2];
	e[1][1] = e[2][2] * e[0][0] - e[2][0] * e[0][2];
	e[1][2] = -e[1][2] * e[0][0] - e[1][0] * e[0][2];

	e[2][0] = e[2][1] * e[1][0] - e[2][0] * e[1][1];
	e[2][1] = -e[2][1] * e[0][0] - e[2][0] * e[0][1];
	e[2][2] = e[1][1] * e[0][0] - e[1][0] * e[0][1];

	(*this) *= 1/det;
}

CMatrix33 CMatrix33::InverseOther() const
{
	CMatrix33 other(*this);
	other.Inverse();
	return other;
}

void CMatrix33::Transpose()
{
	*this = TransposeOther();	
}

CMatrix33 CMatrix33::TransposeOther() const
{
	CMatrix33 other;
	
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			other.e[i][j] = e[j][i];

	return other;
}

CVector3D CMatrix33::operator*(const CVector3D& vec) const
{
	CVector3D ret;
	
	ret.m_X = e[0][0] * vec.m_X + e[0][1] * vec.m_Y + e[0][2] * vec.m_Z;
	ret.m_Y = e[1][0] * vec.m_X + e[1][1] * vec.m_Y + e[1][2] * vec.m_Z;
	ret.m_Z = e[2][0] * vec.m_X + e[2][1] * vec.m_Y + e[2][2] * vec.m_Z;

	return ret;
}

CMatrix33 CMatrix33::operator*(const CMatrix33& other) const
{
	CMatrix33 ret;

	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			ret.e[i][j] = e[i][0] * other.e[0][j] + e[i][1] * other.e[1][j] + e[i][2] * other.e[2][j];

	return ret;
}

CMatrix33 CMatrix33::operator+(const CMatrix33& other) const
{
	CMatrix33 ret;

	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			ret.e[i][j] = e[i][j] + other.e[i][j];

	return ret;
}

CMatrix33 CMatrix33::operator-(const CMatrix33& other) const
{
	CMatrix33 ret;

	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			ret.e[i][j] = e[i][j] - other.e[i][j];

	return ret;
}


CMatrix33 CMatrix33::operator*(btScalar val) const
{
	CMatrix33 ret;

	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			ret.e[i][j] = e[i][j] * val;

	return ret;
}

CMatrix33 CMatrix33::operator/(btScalar val) const
{
	CMatrix33 ret;

	//assert(val != 0);

	btScalar eps = 1e-10;

	if ( 0 <= val && val <= eps )
		val += eps;
	else if ( -eps <= val && val <= 0 )
		val -= eps;

	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			ret.e[i][j] = e[i][j] / val;

	return ret;
}

CMatrix33& CMatrix33::operator*=(btScalar val)
{
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			e[i][j] = val * e[i][j];

	return (*this);
}

CMatrix33& CMatrix33::operator-=(const CMatrix33& other)
{
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			e[i][j] = e[i][j] - other.e[i][j];

	return (*this);
}

CMatrix33& CMatrix33::operator+=(const CMatrix33& other)
{
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			e[i][j] = e[i][j] + other.e[i][j];

	return (*this);
}

CMatrix33& CMatrix33::operator=(const CMatrix33& other)
{
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			e[i][j] = other.e[i][j];

	return (*this);
}

//CMatrix33& CMatrix33::operator=(btScalar a)
//{
//	for ( int i = 0; i < 3; i++ )
//		for ( int j = 0; j < 3; j++ )
//			e[i][j] = a;
//
//	return (*this);
//}

bool CMatrix33::operator==(const CMatrix33& other)
{
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
		{
			if ( e[i][j] != other.e[i][j] )
				return false;
		}

	return true;
}

bool CMatrix33::operator!=(const CMatrix33& other)
{
	return !(*this).operator==(other);
}

bool CMatrix33::operator==(btScalar a)
{
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
		{
			if ( e[i][j] != a )
				return false;
		}

	return true;
}

bool CMatrix33::operator!=(btScalar a)
{
	return !(*this).operator==(a);
}

btScalar& CMatrix33::operator()(int i, int j)
{
	assert(0 <= i && i < 3);
	assert(0 <= i && j < 3);

	return e[i][j];
}

const btScalar& CMatrix33::operator()(int i, int j) const
{
	assert(0 <= i && i < 3);
	assert(0 <= i && j < 3);

	return e[i][j];
}

CMatrix33 operator*(btScalar val, const CMatrix33& other)
{
	return other * val;
}

CMatrix33 operator-(const CMatrix33& other)
{
	CMatrix33 ret;

	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			ret.e[i][j] = -other.e[i][j] ;

	return ret;
}
