#pragma once

#include "Vector3D.h"

#include "BoundingVolume.h"

class CAabb : public IBoundingVolume
{
public:
	CAabb(void);
	CAabb(const CAabb& other);
	virtual ~CAabb(void);

private:
	CVector3D m_Min;
	CVector3D m_Max;

	const CVector3D& Min() const { return m_Min; }
	CVector3D& Min() { return m_Min; }
	const CVector3D& Max() const { return m_Max; }
	CVector3D& Max() { return m_Max; }
	void Set(CVector3D min, CVector3D max);
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

	// Split this box into two CAabb boxes by cutting the longest side half
	virtual void Split(IBoundingVolume*& leftBV, IBoundingVolume*& rightBV) const;

	IBoundingVolume& operator=(const IBoundingVolume& other);
	CAabb& operator=(const CAabb& other);
	IBoundingVolume& operator+=(const CVector3D& vec);
	IBoundingVolume& operator+=(const IBoundingVolume& other);
};
