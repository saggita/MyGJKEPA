#include <math.h>
#include <cassert>
#include "Quaternion.h"

CQuaternion::CQuaternion(double x/* = 0.0*/, double y/* = 0.0*/, double z/* = 0.0*/, double w/* = 1.0*/) : m_X(x), m_Y(y), m_Z(z), m_W(w)
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

CQuaternion::CQuaternion(const CVector3D& axis, double angle_radian)
{
	SetRotation(axis, angle_radian);
}

CQuaternion& CQuaternion::Normalize()
{
	double n = m_W * m_W + m_X * m_X + m_Y * m_Y + m_Z * m_Z;

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

void CQuaternion::SetRotation(const CVector3D& axis, double angle_radian)
{
	// This function assumes that the axis vector has been normalized.
	double halfAng = 0.5 * angle_radian;
    double sinHalf = sin(halfAng);
	m_W = cos(halfAng);

    m_X = sinHalf * axis.m_X;
    m_Y = sinHalf * axis.m_Y;
    m_Z = sinHalf * axis.m_Z;
}

void CQuaternion::SetRotation(const CMatrix33& rotMat)
{
	double fTrace = rotMat.e[0][0]+rotMat.e[1][1]+rotMat.e[2][2];
	double fRoot;

	if ( fTrace > 0.0f )
	{
		// |w| > 1/2, may as well choose w > 1/2
		fRoot = sqrt(fTrace + 1.0f);  // 2w
		m_W = 0.5*fRoot;
		fRoot = 0.5/fRoot;  // 1/(4w)
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
		double* apkQuat[3] = { &m_X, &m_Y, &m_Z };
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

void CQuaternion::GetRotation(CVector3D* pAxis, double* pAngle_radian) const
{
	*pAngle_radian= 2.0f * acos(m_W);

	 double scale = sqrt(m_X * m_X + m_Y * m_Y + m_Z * m_Z);

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
    double nQ = m_X*m_X + m_Y*m_Y + m_Z*m_Z + m_W*m_W;
    double s = 0.0;

    if (nQ > 0.0) {
        s = 2.0/nQ;
    }

	double xs = m_X*s;
    double ys = m_Y*s;
    double zs = m_Z*s;
    double wxs = m_W*xs;
    double wys = m_W*ys;
    double wzs = m_W*zs;
    double xxs = m_X*xs;
    double xys = m_X*ys;
    double xzs = m_X*zs;
    double yys = m_Y*ys;
    double yzs = m_Y*zs;
    double zzs = m_Z*zs;

	pMat33->Set(1.0-yys-zzs, xys-wzs, xzs + wys,
                xys + wzs, 1.0-xxs-zzs, yzs-wxs,
                xzs-wys, yzs + wxs, 1.0-xxs-yys);
}

CMatrix33 CQuaternion::GetMatrix33() const
{
	CMatrix33 mat;
	GetRotation(&mat);
	return mat;
}

double CQuaternion::Length() const
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
	double lengthSqr = m_X*m_X + m_Y*m_Y + m_Z*m_Z + m_W*m_W;

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
	double xxzz = m_X*m_X - m_Z*m_Z;
	double wwyy = m_W*m_W - m_Y*m_Y;

	double xw2 = m_X*m_W*2.0f;
	double xy2 = m_X*m_Y*2.0f;
	double xz2 = m_X*m_Z*2.0f;
	double yw2 = m_Y*m_W*2.0f;
	double yz2 = m_Y*m_Z*2.0f;
	double zw2 = m_Z*m_W*2.0f;

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