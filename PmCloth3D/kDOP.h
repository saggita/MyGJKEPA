#pragma once

#include "Vector3D.h"
#include "BoundingVolume.h"

class CkDOP18 : public IBoundingVolume
{
public:
	CkDOP18(void);
	CkDOP18(const CkDOP18& other);
	CkDOP18(const CVector3D& p);
	virtual ~CkDOP18(void);

private:
	// distances to kDOP planes
	double d[18];

	// Compute the distances to planes with normals from kDOP vectors except those of AABB face planes 
	void Distances(const CVector3D& p, double& d3, double& d4, double& d5, double& d6, double& d7, double& d8) const;
	void Distances(const CVector3D& p, double d[]) const;

public:
	virtual void Enlarge(double h);

	virtual bool Collide(const IBoundingVolume& other, double tolerance = 0) const;
	virtual bool Inside(const CVector3D& point) const;

	virtual void Empty();
	virtual bool IsEmpty() const;
	virtual void Visualize(bool bCollided = false) const;

	virtual double Height() const;
	virtual double Width() const;
	virtual double Length() const;
	virtual CVector3D Center() const;
	virtual double Volume() const;

	// If width is longest, returns 0. If height is longest, returns 1. If length is longest, returns 2. 
	virtual int LongestSide() const;

	// Split this into two CkDOPs by cutting the longest side half
	virtual void Split(IBoundingVolume*& leftBV, IBoundingVolume*& rightBV) const;

	IBoundingVolume& operator=(const IBoundingVolume& other);
	CkDOP18& operator=(const CkDOP18& other);
	IBoundingVolume& operator+=(const CVector3D& vec);
	IBoundingVolume& operator+=(const IBoundingVolume& other);
};

