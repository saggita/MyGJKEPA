#pragma once

#include <assert.h>

#include "Point3D.h"
#include "Vector3D.h"

class CCloth;

class CTriangleCloth3D
{
	friend class CCloth;

public:
	CTriangleCloth3D();
	CTriangleCloth3D(const CTriangleCloth3D& other);
	virtual ~CTriangleCloth3D();

protected:
	int m_Index;
	int m_IndexVrx[3];
	int m_IndexEdge[3];
	int m_IndexNormalVec;
	
public:
	float A; // initial area without any deformation. Calculated before simulation starts and won't change over the simulation.

public:
	int GetVertexIndex(int i) const 
	{
		assert( 0 <= i && i < 3 );

		return m_IndexVrx[i];
	}

	void SetVertexIndex(int i, int vertexIndex)
	{
		assert( 0 <= i && i < 3 );
		 m_IndexVrx[i] = vertexIndex;
	}

	int GetEdgeIndex(int i) const 
	{
		assert( 0 <= i && i < 3 );

		return m_IndexEdge[i];
	}

	int GetIndex() const { return m_Index; }
	void SetIndex(int index) { m_Index = index; }
	int GetNormalVectIndex() const { return m_IndexNormalVec; }
	CVector3D GetPointByBaryCoord(const CCloth* pCloth, float a, float b, float c) const;
	CVector3D GetVelocityByBaryCoord(const CCloth* pCloth, float a, float b, float c) const;
	
	CTriangleCloth3D& operator=(const CTriangleCloth3D& other);
};
