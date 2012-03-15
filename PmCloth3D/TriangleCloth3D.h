#pragma once

#include <assert.h>

#include "Point3D.h"
#include "Vector3D.h"

class CCloth3D;

class CTriangleCloth3D
{
	friend class CCloth3D;

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
	// For force calculation following Baraff's paper
	double A; // initial area without any deformation. Calculated before simulation starts and won't change over the simulation.
	double Asqrt; // square root of A

	// following members are based on Baraff/Witkins paper
	double du1; 
	double du2;
	double dv1;
	double dv2;
	double inv_det; // inverse of determinant

	CVector3D Wu;
	CVector3D Wv;

	double dWu_dX[3];
	double dWv_dX[3];

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
	CVector3D GetPointByBaryCoord(const CCloth3D* pCloth, double a, double b, double c) const;
	CVector3D GetVelocityByBaryCoord(const CCloth3D* pCloth, double a, double b, double c) const;
	
	CTriangleCloth3D& operator=(const CTriangleCloth3D& other);
};
