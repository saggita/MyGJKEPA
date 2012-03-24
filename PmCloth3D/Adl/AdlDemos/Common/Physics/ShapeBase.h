#ifndef SHAPE_BASE_H
#define SHAPE_BASE_H


#include <AdlPrimitives/Math/Math.h>

namespace adl
{

_MEM_CLASSALIGN16
class ShapeBase
{
	public:
		_MEM_ALIGNED_ALLOCATOR16;
		enum Type
		{
			SHAPE_BOX,
			SHAPE_TRIANGLE,
			SHAPE_SIMPLE_CVX,
			SHAPE_SPHERE
		};

		ShapeBase( Type type ) : m_type( type ) {}
		virtual ~ShapeBase(){}

		virtual float4* getVertexBuffer()=0;
		virtual int4* getTriangleBuffer()=0;

		virtual int getNumVertex() const=0;
		virtual int getNumTris() const=0;

		//	out.xyz: normal, out.w: penetration
		virtual bool calcPenetration( const float4& v, float4& out, float maxDistance = 0.f )=0;

		virtual bool operator == (const ShapeBase& in) const = 0;

	public:
		Type m_type;
};

};

#endif

