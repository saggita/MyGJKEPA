#pragma once

#include <Demo.h>
#include <Common/Geometry/Aabb.h>

class BroadphaseDemo : public Demo
{
	public:
		static Demo* createFunc( const Device* deviceData ) { return new BroadphaseDemo(deviceData); }

		BroadphaseDemo(const Device* device);
		~BroadphaseDemo();

		void reset();

		void step(float dt);

		void render();

	public:
		enum
		{
			MAX_PARTICLES = 400,
			PAIR_CAPACITY = MAX_PARTICLES*15,
		};

		float4* m_pos;
		float4* m_vel;
		float4* m_force;
		Aabb* m_aabb;
		struct AabbUint* m_aabbUint;

		float4 m_planes[4];

};

