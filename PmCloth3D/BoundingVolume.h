#pragma once

#include "Vector3D.h"

class IBoundingVolume
{
public:
	IBoundingVolume(void) {}
	virtual ~IBoundingVolume(void) {}

	virtual void Enlarge(double h) = 0;

	virtual bool Collide(const IBoundingVolume& other, double tolerance = 0) const = 0;
	virtual bool Inside(const CVector3D& point) const = 0;

	virtual void Empty() = 0;
	virtual bool IsEmpty() const = 0;
	virtual void Visualize(bool bCollided = false) const = 0;

	virtual double Height() const = 0;
	virtual double Width() const = 0;
	virtual double Length() const = 0;
	virtual CVector3D Center() const = 0;
	virtual double Volume() const = 0;
	virtual int LongestSide() const = 0;

	virtual void Split(IBoundingVolume*& leftBV, IBoundingVolume*& rightBV) const = 0;
};
