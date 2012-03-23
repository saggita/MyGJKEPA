#ifndef __MATRIX44_H__
#define __MATRIX44_H__

#include "../btBulletCollisionCommon.h"

class CVector3D;
class CPoint3D;


class CMatrix44 
{
public:
	CMatrix44();
	CMatrix44(const CMatrix44& other);
	CMatrix44(btScalar r1[4], btScalar r2[4], btScalar r3[4], btScalar r4[4]);
	virtual ~CMatrix44();


	btScalar Row[4][4];

public:
	void SetIdentity();
	void SetRotation(const CVector3D& axis, btScalar ang);
	void SetTranslate(btScalar x, btScalar y, btScalar z);
	//void SetScale(const CPoint3D& pnt, btScalar w, btScalar fact);

	btScalar GetElement(int nRow, int nCol) const;

	CVector3D operator*(const CVector3D& vec) const;
	CPoint3D operator*(const CPoint3D& pt) const;
	CMatrix44 operator*(const CMatrix44& other) const;
	CMatrix44& operator=(const CMatrix44& other);
};

#endif // __MATRIX44_H__