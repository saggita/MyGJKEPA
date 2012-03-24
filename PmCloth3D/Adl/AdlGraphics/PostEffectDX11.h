




class PostEffectSSAO
{
	public:
		__inline
		PostEffectSSAO( const Device* deviceData );

		__inline
		~PostEffectSSAO();

		__inline
		void resize(int width, int height);

		__inline
		void renderPre();

		__inline
		void renderPost();

	public:
		const DeviceDX11* m_deviceData;

		DeviceRenderTargetDX11 m_colorBuffer;
		DeviceRenderTargetDX11 m_posBuffer;
		DeviceRenderTargetDX11 m_normalBuffer;
		DeviceRenderTargetDX11 m_compositBuffer;

		DeviceShaderDX11 m_pixelShader;
		DeviceShaderDX11 m_postPixelShader;
		DeviceShaderDX11 m_multiPixelShader;
		DeviceShaderDX11 m_finalPixelShader;

		DeviceShaderDX11 m_globalPixelShader;
};

PostEffectSSAO::PostEffectSSAO( const Device* deviceData )
{
	ADLASSERT( deviceData->m_type == TYPE_DX11 );
	m_deviceData = (const DeviceDX11*)deviceData;

	{
		char path[128];
		GET_SHADER_FULL_PATH( "PostEffectShader.hlsl", path );
		ShaderUtilsDX11 builder( m_deviceData, path );
		builder.createPixelShader( "PS", m_pixelShader );
		builder.createPixelShader( "MultiPS", m_multiPixelShader );
		builder.createPixelShader( "PostPS", m_postPixelShader );
		builder.createPixelShader( "FinalPS", m_finalPixelShader );
	}
}

PostEffectSSAO::~PostEffectSSAO()
{
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_colorBuffer );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_posBuffer );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_normalBuffer );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_compositBuffer );

	ShaderUtilsDX11::deleteShader( m_pixelShader );
	ShaderUtilsDX11::deleteShader( m_postPixelShader );
	ShaderUtilsDX11::deleteShader( m_multiPixelShader );
	ShaderUtilsDX11::deleteShader( m_finalPixelShader );
}

void PostEffectSSAO::resize(int width, int height)
{
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_colorBuffer );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_posBuffer );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_normalBuffer );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_compositBuffer );

	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, width, height, m_colorBuffer );
	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, width, height, m_posBuffer );
	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, width, height, m_normalBuffer );
	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, width, height, m_compositBuffer );
}

void PostEffectSSAO::renderPre()
{
	m_globalPixelShader = g_defaultPixelShader;
	g_defaultPixelShader = m_multiPixelShader;

	//	swap render target using current depth stencil
	ID3D11RenderTargetView* pOrigRTV = NULL;
	ID3D11DepthStencilView* pOrigDSV = NULL;
	m_deviceData->m_context->OMGetRenderTargets( 1, &pOrigRTV, &pOrigDSV );

	float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f };

	ID3D11RenderTargetView* targets[] = { m_colorBuffer.m_renderTarget,
		m_posBuffer.m_renderTarget, m_normalBuffer.m_renderTarget};

	m_deviceData->m_context->OMSetRenderTargets( 3, targets, pOrigDSV );
	m_deviceData->m_context->ClearRenderTargetView( m_colorBuffer.m_renderTarget, ClearColor );
	m_deviceData->m_context->ClearRenderTargetView( m_posBuffer.m_renderTarget, ClearColor );
	m_deviceData->m_context->ClearRenderTargetView( m_normalBuffer.m_renderTarget, ClearColor );
	m_deviceData->m_context->ClearDepthStencilView( pOrigDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );
}

void PostEffectSSAO::renderPost()
{
	RenderObject quad = createQuad(1);

	DeviceDX11* dd = (DeviceDX11*)m_deviceData;

	ID3D11RenderTargetView* pOrigRTV = NULL;
	ID3D11DepthStencilView* pOrigDSV = NULL;
	dd->m_context->OMGetRenderTargets( 1, &pOrigRTV, &pOrigDSV );

	//	release for the renderPre
	pOrigRTV->Release();
	pOrigDSV->Release();

	D3D11_VIEWPORT oldViewport;
	{	//	copy current viewport and set texture sized viewport
		u32 nViewports = 1;
		dd->m_context->RSGetViewports( &nViewports, &oldViewport );

		D3D11_TEXTURE2D_DESC texDesc;
		m_posBuffer.m_texture->GetDesc( &texDesc );
		D3D11_VIEWPORT vp;
		vp.Width = (float)texDesc.Width;
		vp.Height = (float)texDesc.Height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;

		dd->m_context->RSSetViewports( 1, &vp );
	}

	//	post
	{
		ID3D11RenderTargetView* targets[] = { m_compositBuffer.m_renderTarget };

		dd->m_context->OMSetRenderTargets( 1, targets, pOrigDSV );
		dd->m_context->ClearDepthStencilView( pOrigDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );

		dd->m_context->PSSetShaderResources( 0, 1, &m_colorBuffer.m_srv );
		dd->m_context->PSSetShaderResources( 1, 1, &m_posBuffer.m_srv );
		dd->m_context->PSSetShaderResources( 2, 1, &m_normalBuffer.m_srv );

		renderFullQuad( m_deviceData, &m_postPixelShader, make_float4(0,0,0,0) );

		ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
		dd->m_context->PSSetShaderResources( 0, 3, ppSRVNULL );
	}

	dd->m_context->RSSetViewports( 1, &oldViewport );
/*
	if(m_renderBufIdx < 2)
	{
		float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f } ; // red, green, blue, alpha
		ID3D11RenderTargetView* aRTViews[ 1 ] = { pOrigRTV };
		dd->m_context->OMSetRenderTargets( 1, aRTViews, pOrigDSV );
		dd->m_context->ClearDepthStencilView( pOrigDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );
		dd->m_context->ClearRenderTargetView( pOrigRTV, ClearColor );

		//	render to screen
		DeviceRenderTargetDX11 rt[] = {m_renderTarget1, m_colorBuffer, m_posBuffer, m_depthBuffer, m_normalBuffer}; 
		dd->m_context->PSSetShaderResources( 0, 1, &rt[m_renderBufIdx].m_srv );
		renderFullQuad( m_deviceData, &m_pixelShader );

		ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
		dd->m_context->PSSetShaderResources( 0, 1, ppSRVNULL );
	}
	else
*/
	{
		float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f } ; // red, green, blue, alpha
		ID3D11RenderTargetView* aRTViews[ 1 ] = { pOrigRTV };
		dd->m_context->OMSetRenderTargets( 1, aRTViews, pOrigDSV );
		dd->m_context->ClearDepthStencilView( pOrigDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );
		dd->m_context->ClearRenderTargetView( pOrigRTV, ClearColor );

		//	render to screen
		dd->m_context->PSSetShaderResources( 0, 1, &m_colorBuffer.m_srv );
		dd->m_context->PSSetShaderResources( 1, 1, &m_compositBuffer.m_srv );
		renderFullQuad( m_deviceData, &m_finalPixelShader, make_float4(0,0,0,0) );

		ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
		dd->m_context->PSSetShaderResources( 0, 2, ppSRVNULL );
	}

	pOrigRTV->Release();
	pOrigDSV->Release();

	g_defaultPixelShader = m_globalPixelShader;
}


