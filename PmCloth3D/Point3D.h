#ifndef __POINT3D_H__
#define __POINT3D_H__

#include "Vector3D.h"

class CPoint3D
{
public:
	CPoint3D();
	CPoint3D(double x, double y, double z);
	CPoint3D(const CPoint3D& other);
	CPoint3D(const CVector3D& other);
	virtual ~CPoint3D() {}

	// properties..
	double m_X;
	double m_Y;
	double m_Z;

	virtual void Set(double x, double y, double z);
	virtual void TranslateW(double x, double y, double z);
	virtual void RotateW(const CVector3D& axis, double ang);
	virtual void TransformW(CMatrix44 mat44);

	CPoint3D& operator=(const CPoint3D& other);
	CPoint3D operator+(const CPoint3D& other) const;
	CPoint3D operator+(const CVector3D& other) const;
	CPoint3D operator-(const CPoint3D& other) const;
	bool operator==(const CPoint3D& other);
	bool operator!=(const CPoint3D& other);

	CPoint3D operator*(double val) const;
	friend CPoint3D operator*(double val, const CPoint3D& other);
};

#endif // __POINT3D_H__