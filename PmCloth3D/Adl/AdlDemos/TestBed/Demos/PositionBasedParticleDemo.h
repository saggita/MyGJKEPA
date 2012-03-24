#pragma once

#include <Demo.h>

class PositionBasedParticleDemo : public Demo
{
	public:
		static Demo* createFunc( const Device* deviceData ) { return new PositionBasedParticleDemo(); }

		PositionBasedParticleDemo();
		~PositionBasedParticleDemo();

		void reset();

		void step(float dt);

		void render();

		void solve();

		void keyListener(unsigned char key);

	public:
		enum
		{
			NUM_PARTICLES = 128,
		};

		float4* m_pos;
		float4* m_posPrev;
		float m_radius;
		float4 m_boundary[3];

		Array<int2> m_pairs;
		int m_nIter;

};
