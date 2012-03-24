#pragma once

#include <Demo.h>


class ShadowMapDemo : public Demo
{
	public:
		static Demo* createFunc( const Device* deviceData ) { return new ShadowMapDemo(deviceData); }

		ShadowMapDemo( const Device* device ); 
		~ShadowMapDemo(); 

		void reset();

		void step(float dt);

		void render();

		void renderPost();

	public:
		enum
		{
			MAX_BODIES = 10, 
		};

		struct Constants
		{
			XMMATRIX m_lightView;
			XMMATRIX m_lightProjection;
		};

		class ShapeBase* m_shapes[MAX_BODIES];
		float4 m_pos[MAX_BODIES];

		BufferDX11<Constants> m_smConstBuffer;
		DeviceShaderDX11 m_shadowPShader;
		Buffer<float4> m_dummyBuffer;
		

		DeviceRenderTargetDX11 m_colorRT[2];
		DeviceRenderTargetDX11 m_depthRT;

		ID3D11RenderTargetView* m_rtv;
		ID3D11DepthStencilView* m_dsv;

};

