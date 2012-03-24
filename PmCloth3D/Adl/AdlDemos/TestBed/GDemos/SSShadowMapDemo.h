#pragma once

#include <Demo.h>
#include <GDemos/PostProcessBaseDemo.h>


class SSShadowMapDemo : public PostProcessBaseDemo
{
	public:
		COMPUTE_DX11;

		static Demo* createFunc( const Device* deviceData ) { return new SSShadowMapDemo(deviceData); }

		SSShadowMapDemo(const Device* deviceData);
		~SSShadowMapDemo();

		void initDemo();

		void renderPre();

		void renderPost();
		
	public:
		enum
		{
			MAX_SHADOWS = 20,
		};

		struct ConstData
		{
			XMMATRIX m_viewInv;
			XMMATRIX m_projInv;

			XMMATRIX m_lightView;
			XMMATRIX m_lightProj;

			int m_width;
			int m_height;
			float m_shadowWeight;
			int m_shadowIdx;
		};

		DeviceRenderTargetDX11 m_depthBuffer[MAX_SHADOWS];
		float4 m_lightPos[MAX_SHADOWS];

		Buffer<float4> m_shadowAccumBuffer;
		Buffer<ConstData> m_constBuffer;

		Kernel m_shadowAccmKernel;
		Kernel m_clearKernel;


		Buffer<float> m_lightMergedBuffer;
		Buffer<XMMATRIX> m_lightMatrixBuffer;
		Kernel m_copyShadowMapKernel;
		Kernel m_shadowAccmAllKernel;
};
