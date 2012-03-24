#include <math.h>
#include <cassert>
#include "Quaternion.h"

CQuaternion::CQuaternion(btScalar x/* = 0.0*/, btScalar y/* = 0.0*/, btScalar z/* = 0.0*/, btScalar w/* = 1.0*/) : m_X(x), m_Y(y), m_Z(z), m_W(w)
{
}

CQuaternion::~CQuaternion(void)
{
}

CQuaternion::CQuaternion(const CQuaternion& other)
{	
	m_X = other.m_X;
	m_Y = other.m_Y;
	m_Z = other.m_Z;
	m_W = other.m_W;
}

CQuaternion::CQuaternion(const CMatrix33& rotMat)
{
	SetRotation(rotMat);
}

CQuaternion::CQuaternion(const CVector3D& axis, btScalar angle_radian)
{
	SetRotation(axis, angle_radian);
}

CQuaternion& CQuaternion::Normalize()
{
	btScalar n = m_W * m_W + m_X * m_X + m_Y * m_Y + m_Z * m_Z;

	if ( n == 0 ) 
	{
        m_W = 1;
        return (*this);
    }

    n = 1.0f/sqrt(n);
    
	m_W *= n;
    m_X *= n;
    m_Y *= n;
    m_Z *= n;

	return (*this);
}

void CQuaternion::SetRotation(const CVector3D& axis, btScalar angle_radian)
{
	// This function assumes that the axis vector has been normalized.
	btScalar halfAng = 0.5f * angle_radian;
    btScalar sinHalf = sin(halfAng);
	m_W = cos(halfAng);

    m_X = sinHalf * axis.m_X;
    m_Y = sinHalf * axis.m_Y;
    m_Z = sinHalf * axis.m_Z;
}

void CQuaternion::SetRotation(const CMatrix33& rotMat)
{
	btScalar fTrace = rotMat.e[0][0]+rotMat.e[1][1]+rotMat.e[2][2];
	btScalar fRoot;

	if ( fTrace > 0.0f )
	{
		// |w| > 1/2, may as well choose w > 1/2
		fRoot = sqrt(fTrace + 1.0f);  // 2w
		m_W = 0.5f*fRoot;
		fRoot = 0.5f/fRoot;  // 1/(4w)
		m_X = (rotMat.e[2][1]-rotMat.e[1][2])*fRoot;
		m_Y = (rotMat.e[0][2]-rotMat.e[2][0])*fRoot;
		m_Z = (rotMat.e[1][0]-rotMat.e[0][1])*fRoot;
	}
	else
	{
		// |w| <= 1/2
		static size_t s_iNext[3] = { 1, 2, 0 };
		size_t i = 0;
		if ( rotMat.e[1][1] > rotMat.e[0][0] )
			i = 1;
		if ( rotMat.e[2][2] > rotMat.e[i][i] )
			i = 2;
		size_t j = s_iNext[i];
		size_t k = s_iNext[j];

		fRoot = sqrt(rotMat.e[i][i]-rotMat.e[j][j]-rotMat.e[k][k] + 1.0f);
		btScalar* apkQuat[3] = { &m_X, &m_Y, &m_Z };
		*apkQuat[i] = 0.5f*fRoot;
		fRoot = 0.5f/fRoot;
		m_W = (rotMat.e[k][j]-rotMat.e[j][k])*fRoot;
		*apkQuat[j] = (rotMat.e[j][i]+rotMat.e[i][j])*fRoot;
		*apkQuat[k] = (rotMat.e[k][i]+rotMat.e[i][k])*fRoot;
	}
}

void CQuaternion::SetRotation(const CQuaternion& quaternion)
{
	*this = quaternion;
}

void CQuaternion::GetRotation(CVector3D* pAxis, btScalar* pAngle_radian) const
{
	*pAngle_radian= 2.0f * acos(m_W);

	 btScalar scale = sqrt(m_X * m_X + m_Y * m_Y + m_Z * m_Z);

	 if ( scale > 0 )
	 {
		 pAxis->m_X = m_X / scale;
		 pAxis->m_Y = m_Y / scale;
		 pAxis->m_Z = m_Z / scale;
	 }
	 else
	{
		 pAxis->m_X = 0;
		 pAxis->m_Y = 0;
		 pAxis->m_Z = 0;
	 }
}

void CQuaternion::GetRotation(CMatrix33* pMat33) const
{
    btScalar nQ = m_X*m_X + m_Y*m_Y + m_Z*m_Z + m_W*m_W;
    btScalar s = 0.0;

    if (nQ > 0.0) {
        s = 2.0f/nQ;
    }

	btScalar xs = m_X*s;
    btScalar ys = m_Y*s;
    btScalar zs = m_Z*s;
    btScalar wxs = m_W*xs;
    btScalar wys = m_W*ys;
    btScalar wzs = m_W*zs;
    btScalar xxs = m_X*xs;
    btScalar xys = m_X*ys;
    btScalar xzs = m_X*zs;
    btScalar yys = m_Y*ys;
    btScalar yzs = m_Y*zs;
    btScalar zzs = m_Z*zs;

	pMat33->Set(1.0f-yys-zzs, xys-wzs, xzs + wys,
                xys + wzs, 1.0f-xxs-zzs, yzs-wxs,
                xzs-wys, yzs + wxs, 1.0f-xxs-yys);
}

CMatrix33 CQuaternion::GetMatrix33() const
{
	CMatrix33 mat;
	GetRotation(&mat);
	return mat;
}

btScalar CQuaternion::Length() const
{
	return sqrt(m_X*m_X + m_Y*m_Y + m_Z*m_Z + m_W*m_W);
}

void CQuaternion::SetIdentity()
{
	m_X = m_Y = m_Z = 0.0;
	m_W = 1.0;
}

void CQuaternion::Inverse()
{
	btScalar lengthSqr = m_X*m_X + m_Y*m_Y + m_Z*m_Z + m_W*m_W;

	assert(lengthSqr != 0.0);

	m_X = -m_X / lengthSqr;
	m_Y = -m_Y / lengthSqr;
	m_Z = -m_Z / lengthSqr;
	m_W = m_W / lengthSqr;
}

CQuaternion CQuaternion::InverseOther() const
{
	CQuaternion q(*this);
	q.Inverse();
	return q;
}

CQuaternion& CQuaternion::operator=(const CQuaternion& other)
{
	m_W = other.m_W;
	m_X = other.m_X;
	m_Y = other.m_Y;
	m_Z = other.m_Z;

	return (*this);
}

CQuaternion CQuaternion::operator+(const CQuaternion& other) const
{
	CQuaternion q;

	q.m_W = m_W + other.m_W;
	q.m_X = m_X + other.m_X;
	q.m_Y = m_Y + other.m_Y;
	q.m_Z = m_Z + other.m_Z;

	return q;
}

CQuaternion CQuaternion::operator+(const CVector3D& vec) const
{	
	CQuaternion q;

	q.m_W = m_W;
	q.m_X = m_X + vec.m_X;
	q.m_Y = m_Y + vec.m_Y;
	q.m_Z = m_Z + vec.m_Z;

	return q;
}

CQuaternion CQuaternion::operator* (const CQuaternion& other) const
{
    CQuaternion q(*this);
    
	q.m_W = m_W * other.m_W - m_X * other.m_X - m_Y * other.m_Y - m_Z * other.m_Z;
    q.m_X = m_W * other.m_X + m_X * other.m_W + m_Y * other.m_Z - m_Z * other.m_Y;
    q.m_Y = m_W * other.m_Y + m_Y * other.m_W + m_Z * other.m_X - m_X * other.m_Z;
    q.m_Z = m_W * other.m_Z + m_Z * other.m_W + m_X * other.m_Y - m_Y * other.m_X;
    
	return q;
}

CVector3D CQuaternion::operator* (const CVector3D& vec) const
{
	
	CVector3D uv, uuv;
	CVector3D qvec(m_X, m_Y, m_Z);
	uv = qvec.Cross(vec);
	uuv = qvec.Cross(uv);
	uv *= (2.0f * m_W);
	uuv *= 2.0f;

	return vec + uv + uuv;
	

/*	
	CMatrix33 mat(*this);

	return mat * vec;
*/	
/*
	btScalar xxzz = m_X*m_X - m_Z*m_Z;
	btScalar wwyy = m_W*m_W - m_Y*m_Y;

	btScalar xw2 = m_X*m_W*2.0f;
	btScalar xy2 = m_X*m_Y*2.0f;
	btScalar xz2 = m_X*m_Z*2.0f;
	btScalar yw2 = m_Y*m_W*2.0f;
	btScalar yz2 = m_Y*m_Z*2.0f;
	btScalar zw2 = m_Z*m_W*2.0f;

	return CVector3D(
		(xxzz + wwyy)*vec.m_X		+ (xy2 + zw2)*vec.m_Y		+ (xz2 - yw2)*vec.m_Z,
		(xy2 - zw2)*vec.m_X			+ (xxzz - wwyy)*vec.m_Y		+ (yz2 + xw2)*vec.m_Z,
		(xz2 + yw2)*vec.m_X			+ (yz2 - xw2)*vec.m_Y		+ (wwyy - xxzz)*vec.m_Z
	);*/
}

CVector3D operator*(const CVector3D& vec, const CQuaternion& q)
{
	return q * vec;
}