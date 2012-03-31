#include "TriangleCloth3D.h"
#include "Cloth.h"

CTriangleCloth3D::CTriangleCloth3D()
{
	for ( int i = 0; i < 3; i++ )
	{
		m_IndexVrx[i] = -1;
		m_IndexEdge[i] = -1;			
	}
		
	m_Index = -1;
	m_IndexNormalVec = -1;
}

CTriangleCloth3D::CTriangleCloth3D(const CTriangleCloth3D& other)
{
	for ( int i = 0; i < 3; i++ )
	{
		m_IndexVrx[i] = other.m_IndexVrx[i];
		m_IndexEdge[i] = other.m_IndexEdge[i];
	}

	m_Index = other.m_Index;
	m_IndexNormalVec = other.m_IndexNormalVec;
}

CTriangleCloth3D::~CTriangleCloth3D() 
{
}

CVector3D CTriangleCloth3D::GetPointByBaryCoord(const CCloth* pCloth, double a, double b, double c) const
{
	const CVertexCloth3D& v0 = pCloth->GetVertexArray()[m_IndexVrx[0]];
	const CVertexCloth3D& v1 = pCloth->GetVertexArray()[m_IndexVrx[1]];
	const CVertexCloth3D& v2 = pCloth->GetVertexArray()[m_IndexVrx[2]];

	return CVector3D(v0.m_Pos*a + v1.m_Pos*b + v2.m_Pos*c);
}

CVector3D CTriangleCloth3D::GetVelocityByBaryCoord(const CCloth* pCloth, double a, double b, double c) const
{
	const CVertexCloth3D& v0 = pCloth->GetVertexArray()[m_IndexVrx[0]];
	const CVertexCloth3D& v1 = pCloth->GetVertexArray()[m_IndexVrx[1]];
	const CVertexCloth3D& v2 = pCloth->GetVertexArray()[m_IndexVrx[2]];

	return CVector3D(v0.m_Vel*a + v1.m_Vel*b + v2.m_Vel*c);
}


CTriangleCloth3D& CTriangleCloth3D::operator=(const CTriangleCloth3D& other)
{
	for ( int i = 0; i < 3; i++ )
	{
		m_IndexVrx[i] = other.m_IndexVrx[i];
		m_IndexEdge[i] = other.m_IndexEdge[i];
	}

	m_Index = other.m_Index;
	m_IndexNormalVec = other.m_IndexNormalVec;
	return (*this);
}

