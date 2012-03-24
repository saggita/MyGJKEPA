#pragma once

#include <Demo.h>


class PostProcessBaseDemo : public Demo
{
	public:
		COMPUTE_DX11;

		PostProcessBaseDemo(const Device* deviceData);
		virtual ~PostProcessBaseDemo();

		void init();

		virtual void initDemo() = 0;

		void render();

//		void renderPre();

//		void renderPost();

		void prepareGBuffer();

		void resolve( void** srv );

		void resolveTexture( void** srv );

	public:
		enum
		{
			MAX_BODIES = 10, 
		};

		class ShapeBase* m_shapes[MAX_BODIES];
		float4 m_pos[MAX_BODIES];

		DeviceRenderTargetDX11 m_colorRT;
		DeviceRenderTargetDX11 m_posRT;
		DeviceRenderTargetDX11 m_normalRT;
		
		DeviceShaderDX11 m_gpVShader;
		DeviceShaderDX11 m_gpPShader;
		DeviceShaderDX11 m_gVShader;
		DeviceShaderDX11 m_gPShader;

		ID3D11RenderTargetView* m_rtv;
		ID3D11DepthStencilView* m_dsv;

};


