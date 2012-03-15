#ifndef __POINT2D_H__
#define __POINT2D_H__

#include "Vector2D.h"

class CPoint2D
{
public:
	CPoint2D();
	CPoint2D(double x, double y);
	CPoint2D(const CPoint2D& other);
	CPoint2D(const CVector2D& other);
	virtual ~CPoint2D() {}

	// properties..
	double m_X;
	double m_Y;

	virtual void Set(double x, double y);
	virtual void TranslateW(double x, double y);
	//virtual void RotateW(const CVector3D& axis, double ang);
	//virtual void TransformW(CMatrix44 mat44);

	CPoint2D& operator=(const CPoint2D& other);
	CPoint2D operator+(const CPoint2D& other) const;
	CPoint2D operator-(const CPoint2D& other) const;
	//CPoint2D operator+(const CVector3D& other) const;
	bool operator==(const CPoint2D& other);
	bool operator!=(const CPoint2D& other);

	CPoint2D operator*(double val) const;
	friend CPoint2D operator*(double val, const CPoint2D& other);
};

#endif // __POINT2D_H__