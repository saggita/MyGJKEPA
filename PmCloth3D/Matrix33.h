#pragma once

class CVector3D;

class CMatrix33
{
	friend class CVector3D;
	friend class CQuaternion;

public:
	CMatrix33(void);
	CMatrix33(const CMatrix33& other);
	CMatrix33(double e00, double e01, double e02, double e10, double e11, double e12, double e20, double e21, double e22);
	CMatrix33(double a);
	~CMatrix33(void);

private:
	double e[3][3];

public:
	static const CMatrix33 IDENTITY;
	static const CMatrix33 ZERO;

public:
	void SetIdentity();
	void Set(double e00, double e01, double e02, double e10, double e11, double e12, double e20, double e21, double e22);
	double GetElement(int i, int j) const;
	void SetElement(int i, int j, double val);
	void SetRotation(const CVector3D& axis, double ang);
	void Inverse();
	CMatrix33 InverseOther() const;
	void Transpose();
	CMatrix33 TransposeOther() const;

	CVector3D operator*(const CVector3D& vec) const;
	CMatrix33 operator*(const CMatrix33& other) const;
	CMatrix33 operator*(double val) const;
	CMatrix33 operator+(const CMatrix33& other) const;
	CMatrix33 operator-(const CMatrix33& other) const;
	CMatrix33 operator/(double val) const;
	CMatrix33& operator*=(double val);
	CMatrix33& operator-=(const CMatrix33& other);
	CMatrix33& operator+=(const CMatrix33& other);
	CMatrix33& operator=(const CMatrix33& other);
	//CMatrix33& operator=(double a);

	bool operator==(const CMatrix33& other);
	bool operator!=(const CMatrix33& other);

	bool operator==(double a);
	bool operator!=(double a);

	double& operator()(int i, int j);				
	const double& operator()(int i, int j) const;

	friend CMatrix33 operator*(double val, const CMatrix33& other);
	friend CMatrix33 operator-(const CMatrix33& other);
};
