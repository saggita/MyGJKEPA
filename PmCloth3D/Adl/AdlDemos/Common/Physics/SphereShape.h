#pragma once

#include <common/Physics/ShapeBase.h>

namespace adl
{

class SphereShape : public ShapeBase
{
	public:
		SphereShape( const float rad );
		virtual ~SphereShape();

		virtual float4* getVertexBuffer() { return NULL; }
		virtual int4* getTriangleBuffer() { return NULL; }

		virtual int getNumVertex() const { return 0; }
		virtual int getNumTris() const { return 0; }

		virtual bool calcPenetration( const float4& v, float4& out, float maxDistance = 0.f );

		virtual bool operator == (const ShapeBase& input) const;


	public:
		float m_radius;

};

};
