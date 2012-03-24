#include <GDemos/ShadowMapDemo.h>


extern int g_wWidth;
extern int g_wHeight;

extern XMMATRIX g_ViewTr;
extern XMMATRIX g_ProjectionTr;

ShadowMapDemo::ShadowMapDemo( const Device* device )
	: Demo( device )
{
	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_colorRT[0] );
	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_colorRT[1] );
	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_depthRT, DeviceRenderTargetDX11::TYPE_DEPTH_STENCIL );
	
	{
		m_shapes[0] = new BoxShape( make_float4( 15.f, 2.f, 15.f ) );
		m_pos[0] = make_float4(0.f, -2.f, 0.f);

		float e = 0.1f;
		for(int i=1; i<MAX_BODIES; i++)
		{
			int ii = (i-MAX_BODIES/2);
			ShapeBase* box;
			box = new BoxShape( make_float4(e+0.01f*i,e,e,0) );
			m_shapes[i] = box;
			m_pos[i] = make_float4(ii*e*3.f, e*(i+1)*1.5f, e*ii*3);
		}
	}

	m_smConstBuffer.allocate( m_deviceData, 1, BufferBase::BUFFER_CONST );

	{
		ShaderUtilsDX11 builder( m_deviceData, "GDemos\\ShadowMapShader.hlsl" );
		builder.createPixelShader( "ShadowMapPS", m_shadowPShader );
	}
}

ShadowMapDemo::~ShadowMapDemo()
{
	for(int i=0; i<MAX_BODIES; i++)
	{
		delete m_shapes[i];
	}

	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_colorRT[0] );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_colorRT[1] );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_depthRT );

	ShaderUtilsDX11::deleteShader( m_shadowPShader );
}

void ShadowMapDemo::reset()
{

}

void ShadowMapDemo::step(float dt)
{

}

void ShadowMapDemo::render()
{
	m_dummyBuffer.m_srv = m_depthRT.m_srv;

	for(int i=0; i<MAX_BODIES; i++)
	{
		float c = 0.4f;
		float4 color = make_float4(0.8f-c,0.8f-c,0.8f,1.f)*1.8f;
		color = make_float4(0.5f,1.0f,0.5f,1);

		if( i==0 ) color = make_float4(1.f,1.f,1.f, 1.f);

		drawShape( m_shapes[i], m_pos[i], qtGetIdentity(), color );
	}


	DeviceDX11* dd = (DeviceDX11*)m_deviceData;
	dd->m_context->OMGetRenderTargets( 1, &m_rtv, &m_dsv );

	XMMATRIX viewTr;
	XMMATRIX projTr;
	{	//	render light view
		float ClearColor[4] = { 0.5f, 0.f, 0.f, 0.0f };

		dd->m_context->OMSetRenderTargets( 0, 0, m_depthRT.m_depthStencilView );
		dd->m_context->ClearDepthStencilView( m_depthRT.m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

		getMatrices<true>( make_float4(2,4,0), make_float4(0,0,0), make_float4(0,1,0,0), XM_PI*90.f/180.f, 1.f, 
			0.1f, 100.f, &viewTr, &projTr );
		
		dispatchRenderList( g_deviceData, &g_debugRenderObj, &viewTr, &projTr );
	}

	{	//	render with shadow
		float ClearColor[4] = { 0.5f, 0.f, 0.f, 0.0f };

		ID3D11RenderTargetView* targets[] = { m_colorRT[1].m_renderTarget };
		dd->m_context->OMSetRenderTargets( 1, targets, m_dsv );
		dd->m_context->ClearRenderTargetView( m_colorRT[1].m_renderTarget, ClearColor );
		dd->m_context->ClearDepthStencilView( m_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0 );

		{
			Constants c;
			c.m_lightView = viewTr;
			c.m_lightProjection = projTr;

			dd->m_context->UpdateSubresource( m_smConstBuffer.getBuffer(), 0, 0, &c, 0, 0 );
			dd->m_context->PSSetConstantBuffers(1, 1, m_smConstBuffer.getBufferPtr() );
		}

		for(int i=0; i<g_debugRenderObj.getSize(); i++) 
		{
			g_debugRenderObj[i].m_pixelShader = m_shadowPShader;

			int idx = g_debugRenderObj[i].m_nResources;
			g_debugRenderObj[i].m_resources[idx].m_buffer = (BufferDX11<float4>&)m_dummyBuffer;
			g_debugRenderObj[i].m_resources[idx].m_type = RenderObject::Resource::PIXEL_SHADER;
			g_debugRenderObj[i].m_nResources++;
		}
		dd->m_context->PSSetSamplers( 0, 1, &g_defaultSampler );

		dispatchRenderList( g_deviceData, &g_debugRenderObj, &g_ViewTr, &g_ProjectionTr );

		for(int i=0; i<g_debugRenderObj.getSize(); i++) 
		{
			g_debugRenderObj[i].m_pixelShader = g_defaultPixelShader;
			g_debugRenderObj[i].m_nResources--;
		}
	}

	m_rtv->Release();
	m_dsv->Release();

	dd->m_context->OMSetRenderTargets( 1, &m_rtv, m_dsv );
	dd->m_context->ClearDepthStencilView( m_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0 );
}

extern DeviceShaderDX11 g_textureMapWColorPixelShader;
extern DeviceShaderDX11 g_quadVertexShader;

void ShadowMapDemo::renderPost()
{
	float size = 1.f;
	for(int i=0; i<2; i++)
	{
		DeviceDX11* dd = (DeviceDX11*)m_deviceData;

		float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f };
		ID3D11RenderTargetView* aRTViews[ 1 ] = { m_rtv };
		dd->m_context->OMSetRenderTargets( 1, aRTViews, m_dsv );
		dd->m_context->ClearDepthStencilView( m_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0 );

		if( i==0 )
			dd->m_context->PSSetShaderResources( 0, 1, &m_depthRT.m_srv );
		else
			dd->m_context->PSSetShaderResources( 0, 1, (ID3D11ShaderResourceView**)&m_colorRT[i].m_srv );

		//	render to screen
		{
			RenderObject quad = createQuad1( make_float4(-1,-1+i*size,0), make_float4(size, size, 0), make_float4(0,0,0), make_float4(1,1,1) );
			RenderObject* obj = &quad;
			obj->m_matrix.m_gData = make_float4((float)g_wWidth, (float)g_wHeight, 0,0 );
	
			obj->m_pixelShader = g_textureMapWColorPixelShader;
			obj->m_vertexShader = g_quadVertexShader;

			dd->m_context->PSSetSamplers( 0, 1, &g_defaultSampler );

			{
				Kernel kernel;
				Launcher launcher( m_deviceData, &kernel );
				launcher.setConst<ConstantBuffer>( g_constBuffer, obj->m_matrix );
			}

			obj->render( dd, g_constBuffer.getBuffer() );
		}

		ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
		dd->m_context->PSSetShaderResources( 0, 1, ppSRVNULL );
	}
}
