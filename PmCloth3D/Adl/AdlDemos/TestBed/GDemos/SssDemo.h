#pragma once

#include <Demo.h>

class SssDemo : public Demo
{
	public:
		static Demo* createFunc( const Device* deviceData ) { return new SssDemo(deviceData); }

		SssDemo( const Device* device ); 
		~SssDemo(); 

		void reset();

		void step(float dt);

		void render();

	public:

		enum
		{
			MAX_LIGHTS = 2,
			MAX_BODIES = 2, 

			TEST_SPHERE = 1,
		};

		DeviceShaderDX11 m_sphereSssPShader;
		DeviceShaderDX11 m_boxSssPShader;

		Buffer<float4> m_lightBuffer;
		

		class ShapeBase* m_shapes[MAX_BODIES];
		float4 m_pos[MAX_BODIES];
};

