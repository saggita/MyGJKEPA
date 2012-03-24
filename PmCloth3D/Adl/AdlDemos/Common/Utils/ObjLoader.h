#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <AdlPrimitives/Math/Math.h>
#include <AdlPrimitives/Math/Array.h>
#include <common/Geometry/Aabb.h>


class ObjLoader
{
	public:
		ObjLoader(char* fileName);

		int getNumTriangles()const;
		int getNumVertices()const;
		float4* getVertexBuffer();
		float4* getNormalPerVertexBuffer();
		int4* getNormalIndexBuffer();
		int4* getFaceIndexBuffer();

		static void scale(const Aabb& aabb, float4* vtxInOut, int numVtx);
		static void swapYZ(float4* vtxInOut, int numVtx);
		static void translate(float4* vtxInOut, int numVtx, const float4& translate);
		static void scale(float4* vtxInOut, int numVtx, float scale);
		static void flipWinding(int4* faceInOut, int numTri);

	private:
		void decode(char* buf);
		void readVertex(char* buf);
		void readFace(char* buf);
		static int strchrcount(const char* str, int c);

	private:
		Array<float4> m_vtx;
		Array<float4> m_normals;
		Array<int4> m_faces;
		Array<int4> m_faceNormals;

};



#endif

