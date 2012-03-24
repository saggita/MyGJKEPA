#include <GDemos/PostProcessBaseDemo.h>
#include <common/Physics/BoxShape.h>


extern int g_wWidth;
extern int g_wHeight;

extern DeviceShaderDX11 g_defaultPixelShader;
extern ID3D11SamplerState* g_defaultSampler;
extern BufferDX11<ConstantBuffer> g_constBuffer;
extern DeviceShaderDX11 g_bufferToRTPixelShader;
extern DeviceShaderDX11 g_textureMapWColorPixelShader;

extern XMMATRIX g_ViewTr;
extern XMMATRIX g_ProjectionTr;


PostProcessBaseDemo::PostProcessBaseDemo(const Device* deviceData)
	: Demo()
{
	INITIALIZE_DEVICE_DATA( deviceData );

	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_colorRT );
	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_posRT ); 
	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_normalRT ); 

	{
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		ShaderUtilsDX11 builder( m_deviceData, "GDemos\\Deferred.hlsl" );
		builder.createVertexShader( "DeferredGPVS", m_gpVShader, ARRAYSIZE( layout ), layout );
		builder.createPixelShader( "DeferredGPPS", m_gpPShader );
	}

	m_gVShader = g_defaultVertexShader;
	m_gPShader = g_defaultPixelShader;
}

PostProcessBaseDemo::~PostProcessBaseDemo()
{
	for(int i=0; i<MAX_BODIES; i++)
	{
		delete m_shapes[i];
	}

	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_colorRT );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_posRT );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_normalRT );
	
	ShaderUtilsDX11::deleteShader( m_gpVShader );
	ShaderUtilsDX11::deleteShader( m_gpPShader );

	DESTROY_DEVICE_DATA;
}

void PostProcessBaseDemo::init()
{
	{
		m_shapes[0] = new BoxShape( make_float4( 15.f, 2.f, 15.f ) );
		m_pos[0] = make_float4(0.f, -2.f, 0.f);

		float e = 0.3f;
		for(int i=1; i<MAX_BODIES; i++)
		{
			int ii = (i-MAX_BODIES/2);
			ShapeBase* box;
			box = new BoxShape( make_float4(e,e,e,0) );
			m_shapes[i] = box;
			m_pos[i] = make_float4(ii*e*3.f, e*1.5f, e*ii*3);
		}
	}

	initDemo();
}

void PostProcessBaseDemo::render()
{
	const float4 colors[] = { make_float4(0,1,1,1), make_float4(1,0,1,1), make_float4(1,1,0,1),
		make_float4(0,0,1,1), make_float4(0,1,0,1), make_float4(1,0,0,1) };

	for(int i=0; i<MAX_BODIES; i++)
	{
		float c = 0.4f;
		float4 color = make_float4(0.8f-c,0.8f-c,0.8f,1.f)*1.8f;
		color = make_float4(1.0f,0.5f,0.5f,1);

		if( i==0 ) color = make_float4(1.f,1.f,1.f, 1.f);

		drawShape( m_shapes[i], m_pos[i], qtGetIdentity(), color );
	}
}

void PostProcessBaseDemo::prepareGBuffer()
{
	DeviceDX11* dd = (DeviceDX11*)m_deviceData;

	//	swap render target using current depth stencil
	dd->m_context->OMGetRenderTargets( 1, &m_rtv, &m_dsv );

	float ClearColor[4] = { 0.f, 0.f, 0.f, 0.0f };

	ID3D11RenderTargetView* targets[] = { m_colorRT.m_renderTarget, m_posRT.m_renderTarget, m_normalRT.m_renderTarget };

	dd->m_context->OMSetRenderTargets( 3, targets, m_dsv );
	dd->m_context->ClearRenderTargetView( m_colorRT.m_renderTarget, ClearColor );
	dd->m_context->ClearRenderTargetView( m_posRT.m_renderTarget, ClearColor );
	dd->m_context->ClearRenderTargetView( m_normalRT.m_renderTarget, ClearColor );
	dd->m_context->ClearDepthStencilView( m_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	g_defaultVertexShader = m_gpVShader;
	g_defaultPixelShader = m_gpPShader;
}

void PostProcessBaseDemo::resolve( void** srv )
{
	DeviceDX11* dd = (DeviceDX11*)m_deviceData;

//	ID3D11RenderTargetView* pOrigRTV = NULL;
//	ID3D11DepthStencilView* pOrigDSV = NULL;
//	dd->m_context->OMGetRenderTargets( 1, &pOrigRTV, &pOrigDSV );

	//	release for the renderPre
	m_rtv->Release();
	m_dsv->Release();

	{
		float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f };
		ID3D11RenderTargetView* aRTViews[ 1 ] = { m_rtv };
		dd->m_context->OMSetRenderTargets( 1, aRTViews, m_dsv );
		dd->m_context->ClearDepthStencilView( m_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0 );
		dd->m_context->ClearRenderTargetView( m_rtv, ClearColor );

		dd->m_context->PSSetShaderResources( 0, 1, (ID3D11ShaderResourceView**)srv );

		//	render to screen
		renderFullQuad( m_deviceData, &g_bufferToRTPixelShader, make_float4((float)g_wWidth, (float)g_wHeight, 0,0 ) );

		ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
		dd->m_context->PSSetShaderResources( 0, 1, ppSRVNULL );
	}

//	pOrigRTV->Release();
//	pOrigDSV->Release();

	g_defaultVertexShader = m_gVShader;
	g_defaultPixelShader = m_gPShader;
}

void PostProcessBaseDemo::resolveTexture( void** srv )
{
	DeviceDX11* dd = (DeviceDX11*)m_deviceData;

//	ID3D11RenderTargetView* pOrigRTV = NULL;
//	ID3D11DepthStencilView* pOrigDSV = NULL;
//	dd->m_context->OMGetRenderTargets( 1, &pOrigRTV, &pOrigDSV );

	//	release for the renderPre
	m_rtv->Release();
	m_dsv->Release();

	{
		float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f };
		ID3D11RenderTargetView* aRTViews[ 1 ] = { m_rtv };
		dd->m_context->OMSetRenderTargets( 1, aRTViews, m_dsv );
		dd->m_context->ClearDepthStencilView( m_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0 );
		dd->m_context->ClearRenderTargetView( m_rtv, ClearColor );

		dd->m_context->PSSetShaderResources( 0, 1, (ID3D11ShaderResourceView**)srv );

		//	render to screen
		renderFullQuad( m_deviceData, &g_textureMapWColorPixelShader, make_float4((float)g_wWidth, (float)g_wHeight, 0,0 ) );

		ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
		dd->m_context->PSSetShaderResources( 0, 1, ppSRVNULL );
	}

//	pOrigRTV->Release();
//	pOrigDSV->Release();

	g_defaultVertexShader = m_gVShader;
	g_defaultPixelShader = m_gPShader;
}


