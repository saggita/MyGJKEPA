#pragma once

#include <Demo.h>
#include <DemoCommon/ClothSimulation.h>

class ObjLoader;

class DeformableDemo : public Demo
{
	public:
		static Demo* createFunc( const Device* deviceData ) { return new DeformableDemo(); }

		DeformableDemo();
		~DeformableDemo();

		void reset();

		void step(float dt);

		void render();

	public:
		enum
		{
			MAX_CLOTH = 3,
		};

		float m_clothSize;
		float m_particleDiam;

		Cloth m_cloth[MAX_CLOTH];
		float4 m_boundary;

		ObjLoader* m_obj;
};
