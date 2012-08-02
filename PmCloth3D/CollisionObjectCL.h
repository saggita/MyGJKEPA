#pragma once

#include <vector>
#include "Transform.h"
#include "CollisionObject.h"

struct float4c
{	
	float x,y,z,w;		
};

__inline
float4c ToFloat4c(float x, float y, float z, float w = 0.f)
{
	float4c v;
	v.x = x; v.y = y; v.z = z; v.w = w;
	return v;
}

__inline
float4c ToFloat4c(const CVector3D& vec)
{
	float4c v;
	v.x = vec.m_X; v.y = vec.m_Y; v.z = vec.m_Z; v.w = 0.f;
	return v;
}

//----------------
_MEM_CLASSALIGN16
struct CVertexCL
//----------------
{
	_MEM_ALIGNED_ALLOCATOR16;

	unsigned int m_Index;
	float4c m_Pos;
	unsigned int m_NumIndexFaces;
	unsigned int m_IndexFaces[10];
	unsigned int m_NumIndexEdges;
	unsigned int m_IndexEdges[10];
};

//--------------------
_MEM_CLASSALIGN16
struct CTriangleFaceCL
//--------------------
{
	_MEM_ALIGNED_ALLOCATOR16;

	unsigned int m_Index;
	unsigned int m_IndexVrx[3];
	unsigned int m_IndexEdge[3];

	// If true, a vector formed by two points starting from CEdge::m_IndexVrx[0] and ending at CEdge::m_IndexVrx[1] will be right direction
	// in terms of normal vector of this triangle face. The right direction means three vectors will make the counter-clock-wise orientation
	// around normal vector. If false, swap the two points and it will create the right direction. 
	unsigned int m_WindingOrderEdge[3]; 

	float4c m_PlaneEqn;
};

//----------------
_MEM_CLASSALIGN16
struct CEdgeCL
//----------------
{
	_MEM_ALIGNED_ALLOCATOR16;

	unsigned int m_bFlag; 
	unsigned int m_Index;
	unsigned int m_IndexVrx[2];
	unsigned int m_IndexTriangle[2];
};

//------------------
_MEM_CLASSALIGN16
struct CTransformCL
//------------------
{
	_MEM_ALIGNED_ALLOCATOR16;

	float4c m_Translation;
	float4c m_Rotation;
};

//-----------------------
_MEM_CLASSALIGN16
struct CCollisionObjectCL
//-----------------------
{
	_MEM_ALIGNED_ALLOCATOR16;

	// transforms local to world. 
	CTransformCL m_Transform;

	unsigned int m_NumVertices;
	CVertexCL m_Vertices[20];	
	unsigned int m_NumFaces;
	float4c m_Normals[20];
	CTriangleFaceCL m_Faces[20];
	unsigned int m_NumEdges;
	CEdgeCL m_Edges[20];
};

void MakeCCollisionObjectCL(const CCollisionObject& objA, CCollisionObjectCL* pCObjCL_A);