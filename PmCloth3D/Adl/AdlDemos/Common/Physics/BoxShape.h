#ifndef BOX_SHAPE_H
#define BOX_SHAPE_H

#include <common/Physics/ShapeBase.h>
#include <AdlPrimitives/Math/Matrix3x3.h>

_MEM_CLASSALIGN16
class BoxShape : public ShapeBase
{
	public:
		_MEM_ALIGNED_ALLOCATOR16;

		BoxShape( const float4& halfExtent );
		virtual ~BoxShape();

		virtual float4* getVertexBuffer() { return m_vtx; }
		virtual int4* getTriangleBuffer() { return m_tris; }

		virtual int getNumVertex() const { return 8; }
		virtual int getNumTris() const { return 12; }

		//	out.xyz: normal, out.w: penetration
		virtual bool calcPenetration( const float4& v, float4& out, float maxDistance = 0.f );

		virtual bool operator == (const ShapeBase& input) const;

		float4 getExtent() const { return m_halfExtent*2.f; }

		void get6FaceQuads(int4* idxOut);

		static Matrix3x3 calcInertia(const float4& extent, float mass);


	private:
		float4 m_vtx[8];
		int4 m_tris[12];
		float4 m_normals[6];

		float4 m_halfExtent;
};



#endif

