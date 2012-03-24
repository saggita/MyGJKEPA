#pragma once

#include <Demo.h>
#include <Common/Geometry/Aabb.h>
#include <GDemos/PostProcessBaseDemo.h>


#if defined(DX11RENDER)
class ShapeBase;

class ClusteredDeferredDemo : public PostProcessBaseDemo
{
	public:
		COMPUTE_DX11;

		static Demo* createFunc( const Device* deviceData ) { return new ClusteredDeferredDemo(deviceData); }

		ClusteredDeferredDemo(const Device* deviceData);
		~ClusteredDeferredDemo();

		void initDemo();

		void renderPre();

		void renderPost();

		int calcNumTiles(int size, int clusterSize);

	public:
		enum
		{
			MAX_LIGHTS = 4096,

			NUM_TILES_PER_CLUSTER = 1, 
			TILE_SIZE = 8, 
			CLUSTER_SIZE = (NUM_TILES_PER_CLUSTER*TILE_SIZE),

			MAX_LIGHTS_PER_TILE = 128,
			MAX_LIGHTS_PER_TILE_IN_32B = MAX_LIGHTS_PER_TILE/32
		};

		Aabb m_clusterAabbs[MAX_LIGHTS/MAX_LIGHTS_PER_TILE];

		Kernel m_kernel;
		Buffer<float4> m_buffer;


		Buffer<float4> m_lightClusterBuffer;
		Buffer<float4> m_lightPosBuffer;
		Buffer<float4> m_lightColorBuffer;

		bool m_firstFrame;
};
#else
class ClusteredDeferredDemo : public Demo
{
	public:
		static Demo* createFunc( const DeviceDataBase* deviceData ) { return new ClusteredDeferredDemo(deviceData); }

		ClusteredDeferredDemo(const DeviceDataBase* deviceData){}
		~ClusteredDeferredDemo(){}
};

#endif
