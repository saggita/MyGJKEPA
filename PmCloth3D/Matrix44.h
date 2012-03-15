#ifndef __MATRIX44_H__
#define __MATRIX44_H__

class CVector3D;
class CPoint3D;

class CMatrix44 
{
public:
	CMatrix44();
	CMatrix44(const CMatrix44& other);
	CMatrix44(double r1[4], double r2[4], double r3[4], double r4[4]);
	virtual ~CMatrix44();


	double Row[4][4];

public:
	void SetIdentity();
	void SetRotation(const CVector3D& axis, double ang);
	void SetTranslate(double x, double y, double z);
	//void SetScale(const CPoint3D& pnt, double w, double fact);

	double GetElement(int nRow, int nCol) const;

	CVector3D operator*(const CVector3D& vec) const;
	CPoint3D operator*(const CPoint3D& pt) const;
	CMatrix44 operator*(const CMatrix44& other) const;
	CMatrix44& operator=(const CMatrix44& other);
};

#endif // __MATRIX44_H__