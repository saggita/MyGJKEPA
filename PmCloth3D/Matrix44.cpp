#include <cmath>
#include "Matrix44.h"
#include "mathUtil.h"
#include "Vector3D.h"
#include "Point3D.h"

#define vsin(x) ((1.0f) - cos(x)) 

CMatrix44::CMatrix44()
{
	int i = 0;
	int j = 0;

	for ( i = 0; i < 4; i++ )
	{
		for ( j = 0; j < 4; j++ )
			Row[i][j] = 0.0f;
	}
}

CMatrix44::CMatrix44(const CMatrix44& other)
{
	int i = 0;
	int j = 0;

	for ( i = 0; i < 4; i++ )
	{
		for ( j = 0; j < 4; j++ )
			Row[i][j] = other.Row[i][j];
	}
}

CMatrix44::CMatrix44(btScalar R1[4], btScalar R2[4], btScalar R3[4], btScalar R4[4])
{
	int i;

	for ( i = 0; i < 4; i++ )
	{
		Row[0][i] = R1[i];
	}

	for ( i = 0; i < 4; i++ )
	{
		Row[1][i] = R2[i];
	}

	for ( i = 0; i < 4; i++ )
	{
		Row[2][i] = R3[i];
	}

	for ( i = 0; i < 4; i++ )
	{
		Row[3][i] = R4[i];
	}
}

CMatrix44::CMatrix44(btScalar e00, btScalar e01, btScalar e02, btScalar e03,
			  btScalar e10, btScalar e11, btScalar e12, btScalar e13,
			  btScalar e20, btScalar e21, btScalar e22, btScalar e23,
			  btScalar e30, btScalar e31, btScalar e32, btScalar e33)
{
	Row[0][0] = e00; Row[0][1] = e01; Row[0][2] = e02; Row[0][3] = e03;
	Row[1][0] = e10; Row[1][1] = e11; Row[1][2] = e12; Row[1][3] = e13;
	Row[2][0] = e20; Row[2][1] = e21; Row[2][2] = e22; Row[2][3] = 203;
	Row[3][0] = e30; Row[3][1] = e31; Row[3][2] = e32; Row[3][3] = e33;
}

CMatrix44::~CMatrix44()
{
}

void CMatrix44::SetIdentity()
{
	Row[0][0] = 1.0f; Row[0][1] = 0.0f; Row[0][2] = 0.0f; Row[0][3] = 0.0f;
	Row[1][0] = 0.0f; Row[1][1] = 1.0f; Row[1][2] = 0.0f; Row[1][3] = 0.0f;
	Row[2][0] = 0.0f; Row[2][1] = 0.0f; Row[2][2] = 1.0f; Row[2][3] = 0.0f;
	Row[3][0] = 0.0f; Row[3][1] = 0.0f; Row[3][2] = 0.0f; Row[3][3] = 1.0f;
}

void CMatrix44::SetRotation(const CVector3D& axis, btScalar ang)
{
	btScalar nx = axis.m_X; 
	btScalar ny = axis.m_Y;
	btScalar nz = axis.m_Z;

	Row[0][0] = nx*nx*vsin(ang) + cos(ang);
	Row[0][1] = nx*ny*vsin(ang) - nz*sin(ang);
	Row[0][2] = nx*nz*vsin(ang) + ny*sin(ang);
	
	Row[1][0] = nx*ny*vsin(ang) + nz*sin(ang);
	Row[1][1] = ny*ny*vsin(ang) + cos(ang);
	Row[1][2] = ny*nz*vsin(ang) - nx*sin(ang);

	Row[2][0] = nx*nz*vsin(ang) - ny*sin(ang);
	Row[2][1] = ny*nz*vsin(ang) + nx*sin(ang);
	Row[2][2] = nz*nz*vsin(ang) + cos(ang);

	Row[3][0] = 0.0f;
	Row[3][1] = 0.0f;
	Row[3][2] = 0.0f;
	Row[3][3] = 1.0f;
}

void CMatrix44::SetTranslate(btScalar x, btScalar y, btScalar z)
{
	Row[0][3] = x;
	Row[1][3] = y;
	Row[2][3] = z;
}

btScalar CMatrix44::GetElement(int nRow, int nCol) const 
{ 
	if ( nRow < 0 || nRow > 3 )
		throw 0; // needs to be rewriten

	if ( nCol < 0 || nCol > 3 )
		throw 0; // needs to be rewriten

	return Row[nRow][nCol]; 
}

CVector3D CMatrix44::operator*(const CVector3D &vec) const
{
	CVector3D retVal;

	retVal.m_X = Row[0][0]*vec.m_X + Row[0][1]*vec.m_Y + Row[0][2]*vec.m_Z + Row[0][3];
	retVal.m_Y = Row[1][0]*vec.m_X + Row[1][1]*vec.m_Y + Row[1][2]*vec.m_Z + Row[1][3];
	retVal.m_Z = Row[2][0]*vec.m_X + Row[2][1]*vec.m_Y + Row[2][2]*vec.m_Z + Row[2][3];

	return retVal;
}

CPoint3D CMatrix44::operator*(const CPoint3D &pt) const
{
	CPoint3D retVal;

	retVal.m_X = Row[0][0]*pt.m_X + Row[0][1]*pt.m_Y + Row[0][2]*pt.m_Z + Row[0][3];
	retVal.m_Y = Row[1][0]*pt.m_X + Row[1][1]*pt.m_Y + Row[1][2]*pt.m_Z + Row[1][3];
	retVal.m_Z = Row[2][0]*pt.m_X + Row[2][1]*pt.m_Y + Row[2][2]*pt.m_Z + Row[2][3];

	return retVal;
}

CMatrix44 CMatrix44::operator*(const CMatrix44& other) const
{
	CMatrix44 retMat;

	for ( int i = 0; i < 4; i++ )
	{
		for ( int j = 0; j < 4; j++ )
			retMat.Row[i][j] = Row[i][0] * other.Row[0][j] + Row[i][1] * other.Row[1][j] + Row[i][2] * other.Row[2][j] + Row[i][3] * other.Row[3][j];
	}

	return retMat;
}

CMatrix44& CMatrix44::operator=(const CMatrix44& other)
{
	int i = 0;
	int j = 0;

	for ( i = 0; i < 4; i++ )
	{
		for ( j = 0; j < 4; j++ )
			Row[i][j] = other.Row[i][j];
	}

	return *this;
}