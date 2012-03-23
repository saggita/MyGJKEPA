#pragma once

#include "Matrix33.h"
#include "Vector3D.h"

class CQuaternion
{
public:
	CQuaternion(btScalar x = 0.0, btScalar y = 0.0, btScalar z = 0.0, btScalar w = 1.0);
	CQuaternion(const CQuaternion& other);	
	CQuaternion(const CMatrix33& rotMat);
	CQuaternion(const CVector3D& axis, btScalar angle_radian);
	~CQuaternion(void);

public:	
	btScalar m_X;
	btScalar m_Y;
	btScalar m_Z; 
	btScalar m_W;

	CQuaternion& Normalize();

	void SetRotation(const CVector3D& axis, btScalar angle_radian);
	void SetRotation(const CMatrix33& rotMat);
	void SetRotation(const CQuaternion& quaternion);
	void GetRotation(CVector3D* pAxis, btScalar* pAngle_radian) const;
	void GetRotation(CMatrix33* pMat33) const;
	CMatrix33 GetMatrix33() const;
	btScalar Length() const;

	void SetIdentity();
	void Inverse();
	CQuaternion InverseOther() const;

	CQuaternion& operator=(const CQuaternion& other);
	CQuaternion operator+(const CQuaternion& other) const;
	CQuaternion operator+(const CVector3D& vec) const;
	CQuaternion operator* (const CQuaternion& other) const;
	CVector3D operator* (const CVector3D& vec) const;

	friend CVector3D operator*(const CVector3D& vec, const CQuaternion& q);
};
