#pragma once

#include <Demo.h>

#if defined(DX11RENDER)
class ShapeBase;

class SimpleDeferredDemo : public Demo
{
	public:
		COMPUTE_DX11;

		static Demo* createFunc( const Device* deviceData ) { return new SimpleDeferredDemo(deviceData); }

		SimpleDeferredDemo(const Device* deviceData);
		~SimpleDeferredDemo();

		void init();

		void render();

		void renderPre();

		void renderPost();

		int calcNumTiles(int size, int clusterSize);

	public:
		enum
		{
			MAX_BODIES = 10, 
			MAX_LIGHTS = 256,

			NUM_TILES_PER_CLUSTER = 8, 
			TILE_SIZE = 8, 
			CLUSTER_SIZE = (NUM_TILES_PER_CLUSTER*TILE_SIZE),

			MAX_LIGHTS_PER_TILE = 128,
			MAX_LIGHTS_PER_TILE_IN_32B = MAX_LIGHTS_PER_TILE/32
		};

		ShapeBase* m_shapes[MAX_BODIES];
		float4 m_pos[MAX_BODIES];


		DeviceRenderTargetDX11 m_colorRT;
		DeviceRenderTargetDX11 m_posRT;
		DeviceRenderTargetDX11 m_normalRT;
		
		DeviceShaderDX11 m_gpVShader;
		DeviceShaderDX11 m_gpPShader;
		DeviceShaderDX11 m_gVShader;
		DeviceShaderDX11 m_gPShader;

		DeviceShaderDX11 m_pixelShader;

		Kernel m_kernel;
		Buffer<float4> m_buffer;

		Kernel m_clearLightIdxKernel;
		Kernel m_buildLightIdxKernel;
		Buffer<u32> m_tileBuffer;

		Buffer<float4> m_lightPosBuffer;
		Buffer<float4> m_lightColorBuffer;

		bool m_firstFrame;
};
#else
class SimpleDeferredDemo : public Demo
{
	public:
		static Demo* createFunc( const DeviceDataBase* deviceData ) { return new SimpleDeferredDemo(deviceData); }

		SimpleDeferredDemo(const DeviceDataBase* deviceData){}
		~SimpleDeferredDemo(){}
};
#endif
