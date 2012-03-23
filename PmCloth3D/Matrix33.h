#pragma once

#include "../btBulletCollisionCommon.h"

class CVector3D;

class CMatrix33
{
	friend class CVector3D;
	friend class CQuaternion;

public:
	CMatrix33(void);
	CMatrix33(const CMatrix33& other);
	CMatrix33(btScalar e00, btScalar e01, btScalar e02, btScalar e10, btScalar e11, btScalar e12, btScalar e20, btScalar e21, btScalar e22);
	CMatrix33(btScalar a);
	~CMatrix33(void);

private:
	btScalar e[3][3];

public:
	static const CMatrix33 IDENTITY;
	static const CMatrix33 ZERO;

public:
	void SetIdentity();
	void Set(btScalar e00, btScalar e01, btScalar e02, btScalar e10, btScalar e11, btScalar e12, btScalar e20, btScalar e21, btScalar e22);
	btScalar GetElement(int i, int j) const;
	void SetElement(int i, int j, btScalar val);
	void SetRotation(const CVector3D& axis, btScalar ang);
	void Inverse();
	CMatrix33 InverseOther() const;
	void Transpose();
	CMatrix33 TransposeOther() const;

	CVector3D operator*(const CVector3D& vec) const;
	CMatrix33 operator*(const CMatrix33& other) const;
	CMatrix33 operator*(btScalar val) const;
	CMatrix33 operator+(const CMatrix33& other) const;
	CMatrix33 operator-(const CMatrix33& other) const;
	CMatrix33 operator/(btScalar val) const;
	CMatrix33& operator*=(btScalar val);
	CMatrix33& operator-=(const CMatrix33& other);
	CMatrix33& operator+=(const CMatrix33& other);
	CMatrix33& operator=(const CMatrix33& other);
	//CMatrix33& operator=(btScalar a);

	bool operator==(const CMatrix33& other);
	bool operator!=(const CMatrix33& other);

	bool operator==(btScalar a);
	bool operator!=(btScalar a);

	btScalar& operator()(int i, int j);				
	const btScalar& operator()(int i, int j) const;

	friend CMatrix33 operator*(btScalar val, const CMatrix33& other);
	friend CMatrix33 operator-(const CMatrix33& other);
};
