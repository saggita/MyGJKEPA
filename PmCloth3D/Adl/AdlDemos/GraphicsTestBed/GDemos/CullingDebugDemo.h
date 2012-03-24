#pragma once

#include <Demo.h>
#include <GDemos/PostProcessBaseDemo.h>


class CullingDebugDemo : public PostProcessBaseDemo
{
	public:
		COMPUTE_DX11;

		static Demo* createFunc( const Device* deviceData ) { return new CullingDebugDemo(deviceData); }

		CullingDebugDemo(const Device* deviceData);
		~CullingDebugDemo();

		void initDemo();

		void renderPre();

		void renderPost();

	public:
		enum
		{
			MAX_LIGHTS = 2048,

			ENABLE_PIXEL_CULLING = 0,
		};

		struct ConstData
		{
			XMMATRIX m_view;
			XMMATRIX m_projection;
			XMMATRIX m_projectionInv;
			int m_width;
			int m_height;
			int m_tileRes;
			int m_padding;

		};

		Buffer<float4> m_renderBuffer;

		Kernel m_displayKernel;
		Kernel m_displayPixelKernel;

		Buffer<float4> m_lightBuffer;
		Buffer<ConstData> m_constBuffer;

		class CSCulling* m_culling;
};

