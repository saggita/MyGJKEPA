#ifndef SIMPLE_CONVEX_SHAPE_H
#define SIMPLE_CONVEX_SHAPE_H

#include <Common/Physics/ShapeBase.h>
#include <AdlPrimitives/Math/Quaternion.h>

_MEM_CLASSALIGN16
class SimpleConvexShape : public ShapeBase
{
	public:
		_MEM_ALIGNED_ALLOCATOR16;

		SimpleConvexShape() : ShapeBase( SHAPE_SIMPLE_CVX ), m_nVtx(0), m_nFaces(0){}
		~SimpleConvexShape() {}

		virtual float4* getVertexBuffer();
		virtual int4* getTriangleBuffer();

		virtual int getNumVertex() const;
		virtual int getNumTris() const;

		//	out.xyz: normal, out.w: penetration
		virtual bool calcPenetration( const float4& v, float4& out, float maxDistance = 0.f );

		virtual bool operator == (const ShapeBase& input) const;

		bool set(const float4& pos, const Quaternion& quat, 
			const float4* vtx, int nVtx, const int4* faces, int nFaces);

		static SimpleConvexShape* createRandomShape( float cvxRadius, float minRadius = 0.f, int nPoints = 10 );

	public:
		enum
		{
			MAX_VTX = 20, 
			MAX_FACES = 20,
		};

		float4 m_pos;
		Quaternion m_quat;

		float4 m_vtx[MAX_VTX];
		int4 m_faces[MAX_FACES];
		int m_nVtx;
		int m_nFaces;
		void* m_userData;
};


#endif
