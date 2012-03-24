#pragma once

#include <Demo.h>

class ShallowWaterEqn1DDemo : public Demo
{
	public:
		static Demo* createFunc( const Device* deviceData ) { return new ShallowWaterEqn1DDemo(); }

		ShallowWaterEqn1DDemo();
		~ShallowWaterEqn1DDemo();

		void reset();

		void step(float dt);

		void render();

		float getSurface(int i) { return m_wHeight[i]+m_gHeight[i]; }
		float getPosition(int i) { return i*m_dx; }
		bool cellIsDry(int i) { return m_wHeight[i]<FLT_EPSILON; }

		float4 interpolate( float4* v, int i, float di );

	public:
		enum
		{
			RESOLUTION = 128,
		};

		float m_dx;
		float m_g;

		float4* m_v;
		float* m_wHeight;
		float* m_gHeight;

};

