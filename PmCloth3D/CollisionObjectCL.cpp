#include "CollisionObjectCL.h"

void MakeCCollisionObjectCL(const CCollisionObject& objA, CCollisionObjectCL* pCObjCL_A)
{
	// Copy CCollisionObject to CCollisionObjectCL 
	pCObjCL_A->m_NumVertices = (unsigned int)objA.GetVertices().size();
	pCObjCL_A->m_NumFaces = (unsigned int)objA.GetFaces().size();
	pCObjCL_A->m_NumEdges = (unsigned int)objA.GetEdges().size();

	// vertices
	for ( int i = 0; i < (int)objA.GetVertices().size(); i++ )
	{
		CVertexCL vertCL;
		const CVertex& vert = objA.GetVertices()[i];

		vertCL.m_Index = i;
		vertCL.m_Pos = ToFloat4c(vert);
		vertCL.m_NumIndexEdges = (unsigned int)vert.GetEdgeIndeces().size();
		vertCL.m_NumIndexFaces = (unsigned int)vert.GetFaceIndeces().size();

		for ( int j = 0; j < (int)vert.GetEdgeIndeces().size(); j++ )
			vertCL.m_IndexEdges[j] = vert.GetEdgeIndeces()[j];

		for ( int j = 0; j < (int)vert.GetFaceIndeces().size(); j++ )
			vertCL.m_IndexFaces[j] = vert.GetFaceIndeces()[j];

		pCObjCL_A->m_Vertices[i] = vertCL;
	}

	// faces and normals
	for ( int i = 0; i < (int)objA.GetFaces().size(); i++ )
	{
		CTriangleFaceCL faceCL;
		const CTriangleFace& face = objA.GetFaces()[i];

		faceCL.m_Index = face.GetIndex();

		for ( int j = 0; j < 3; j++ )
			faceCL.m_IndexVrx[j] = face.GetVertexIndex(j);

		for ( int j = 0; j < 3; j++ )
			faceCL.m_IndexEdge[j] = face.GetEdgeIndex(j);

		for ( int j = 0; j < 3; j++ )
			faceCL.m_WindingOrderEdge[j] = face.GetWindingOrderEdge(j);

		faceCL.m_PlaneEqn = ToFloat4c(face.PlaneEquation()[0], face.PlaneEquation()[1], face.PlaneEquation()[2], face.PlaneEquation()[3]);
		pCObjCL_A->m_Faces[i] = faceCL;
		pCObjCL_A->m_Normals[i] = ToFloat4c(face.GetNormal());
	}

	// edges
	for ( int i = 0; i < (int)objA.GetEdges().size(); i++ )
	{
		CEdgeCL edgeCL;
		const CEdge& edge = objA.GetEdges()[i];

		edgeCL.m_Index = edge.GetIndex();
		
		for ( int j = 0; j < 2; j++ )
			edgeCL.m_IndexVrx[j] = edge.GetVertexIndex(j);

		for ( int j = 0; j < 2; j++ )
			edgeCL.m_IndexTriangle[j] = edge.GetTriangleIndex(j);

		pCObjCL_A->m_Edges[i] = edgeCL;
	}
}