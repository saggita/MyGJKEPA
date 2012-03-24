#include <GDemos/SSShadowMapDemo.h>

extern int g_wWidth;
extern int g_wHeight;

extern XMMATRIX g_ViewTr;
extern XMMATRIX g_ProjectionTr;

extern DeviceRenderTargetDX11 g_depthStencil;


SSShadowMapDemo::SSShadowMapDemo(const Device* device)
	: PostProcessBaseDemo( device )
{

	for(int i=0; i<MAX_SHADOWS; i++)
	{
		DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_depthBuffer[i], 
			DeviceRenderTargetDX11::TYPE_DEPTH_STENCIL );
	}

	m_shadowAccumBuffer.allocate( m_deviceData, g_wWidth*g_wHeight );
	m_constBuffer.allocate( m_deviceData, 1, BufferBase::BUFFER_CONST );

	m_lightMergedBuffer.allocate( m_deviceData, g_wWidth*g_wHeight*MAX_SHADOWS );
	m_lightMatrixBuffer.allocate( m_deviceData, MAX_SHADOWS );

	{
		//ShadowAccmKernel
		KernelBuilder<TYPE_DX11> builder;
		builder.setFromFile( m_deviceData, "GDemos\\SSShadowMapKernels", 0, true );
		builder.createKernel("ShadowAccmKernel", m_shadowAccmKernel );
		builder.createKernel("ClearKernel", m_clearKernel );
		builder.createKernel("CopyShadowMapKernel", m_copyShadowMapKernel );
		builder.createKernel("ShadowAccmAllKernel", m_shadowAccmAllKernel );
	}
}

SSShadowMapDemo::~SSShadowMapDemo()
{
	for(int i=0; i<MAX_SHADOWS; i++)
	{
		DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_depthBuffer[i] );
	}

	KernelBuilder<TYPE_DX11>::deleteKernel( m_shadowAccmKernel );
	KernelBuilder<TYPE_DX11>::deleteKernel( m_clearKernel );
	KernelBuilder<TYPE_DX11>::deleteKernel( m_copyShadowMapKernel );
	KernelBuilder<TYPE_DX11>::deleteKernel( m_shadowAccmAllKernel );
}

void SSShadowMapDemo::initDemo()
{
	float4 source = make_float4( 8,8,-8 );
	float s = 4.5f;
	for(int i=0; i<MAX_SHADOWS; i++)
	{
		m_lightPos[i] = source + 
			make_float4( getRandom( -s, s ), 0, getRandom( -s, s ) );
	}
}

void SSShadowMapDemo::renderPre()
{
	prepareGBuffer();
}

void SSShadowMapDemo::renderPost()
{
	Stopwatch sw( m_deviceData );
	if(1)
	{
		sw.start();

		ConstData cb;
		{
			XMVECTOR v;
//			cb.m_viewInv = XMMatrixInverse( &v, g_ViewTr );
//			cb.m_projInv = XMMatrixInverse( &v, g_ProjectionTr );
			cb.m_viewInv = XMMatrixInverse( &v, XMMatrixMultiply( g_ProjectionTr, g_ViewTr ) );

			cb.m_width = g_wWidth;
			cb.m_height = g_wHeight;
			cb.m_shadowWeight = 0.6f/MAX_SHADOWS;
		}

		{	//	clear
			Buffer<int> normalBuffer; normalBuffer.m_srv = m_normalRT.m_srv;
			BufferInfo bInfo[] = { BufferInfo( &m_shadowAccumBuffer ), 
				BufferInfo( &normalBuffer, true ) };
			Launcher launcher( m_deviceData, &m_clearKernel );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( m_constBuffer, cb );
			launcher.launch1D( g_wWidth*g_wHeight, 64 );
		}

		ID3D11RenderTargetView* m_rtv;
		ID3D11DepthStencilView* m_dsv;
		DeviceDX11* dd = (DeviceDX11*)m_deviceData;
		dd->m_context->OMGetRenderTargets( 1, &m_rtv, &m_dsv );

		for(int lightIdx=0; lightIdx<MAX_SHADOWS; lightIdx++)
		{
			XMMATRIX viewTr;
			XMMATRIX projTr;
			{	//	render light view
				dd->m_context->OMSetRenderTargets( 0, 0, m_depthBuffer[lightIdx].m_depthStencilView );
				dd->m_context->ClearDepthStencilView( m_depthBuffer[lightIdx].m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

				getMatrices<true>( m_lightPos[lightIdx], make_float4(0,0,0), make_float4(1,0,0,0), XM_PI*60.f/180.f, 1.f, 
					0.1f, 50.f, &viewTr, &projTr );
		
				dispatchRenderList( g_deviceData, &g_debugRenderObj, &viewTr, &projTr );
			}

			{	//	run compute shader for accumulation
				dd->m_context->OMSetRenderTargets( 0, 0, 0 );

				{
//					cb.m_lightView = viewTr;
//					cb.m_lightProj = projTr;
					//	== mul( mul( v, view ), proj ) in shader (Matrices are transposed)
					cb.m_lightView = XMMatrixMultiply( projTr, viewTr );
				}

				Buffer<int> depthBuffer;	depthBuffer.m_srv = g_depthStencil.m_srv;
				Buffer<int> shadowBuffer;	shadowBuffer.m_srv = m_depthBuffer[lightIdx].m_srv;

				BufferInfo bInfo[] = { BufferInfo( &m_shadowAccumBuffer ), 
					BufferInfo( &depthBuffer, true ), BufferInfo( &shadowBuffer, true ) };

				Launcher launcher( m_deviceData, &m_shadowAccmKernel );
				launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
				launcher.setConst( m_constBuffer, cb );
				launcher.launch2D( g_wWidth, g_wHeight, 8, 8 );
			}
		}

		m_rtv->Release();
		m_dsv->Release();
	}
	else
	{
		ConstData cb;
		{
			XMVECTOR v;
//			cb.m_viewInv = XMMatrixInverse( &v, g_ViewTr );
//			cb.m_projInv = XMMatrixInverse( &v, g_ProjectionTr );
			cb.m_viewInv = XMMatrixInverse( &v, XMMatrixMultiply( g_ProjectionTr, g_ViewTr ) );

			cb.m_width = g_wWidth;
			cb.m_height = g_wHeight;
			cb.m_shadowWeight = 0.6f/MAX_SHADOWS;
		}

		{	//	clear
			Buffer<int> normalBuffer; normalBuffer.m_srv = m_normalRT.m_srv;
			BufferInfo bInfo[] = { BufferInfo( &m_shadowAccumBuffer ), 
				BufferInfo( &normalBuffer, true ) };
			Launcher launcher( m_deviceData, &m_clearKernel );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( m_constBuffer, cb );
			launcher.launch1D( g_wWidth*g_wHeight, 64 );
		}

		ID3D11RenderTargetView* m_rtv;
		ID3D11DepthStencilView* m_dsv;
		DeviceDX11* dd = (DeviceDX11*)m_deviceData;
		dd->m_context->OMGetRenderTargets( 1, &m_rtv, &m_dsv );

		XMMATRIX viewTr[MAX_SHADOWS];
		XMMATRIX projTr[MAX_SHADOWS];
		for(int lightIdx=0; lightIdx<MAX_SHADOWS; lightIdx++)
		{
			{	//	render light view
				dd->m_context->OMSetRenderTargets( 0, 0, m_depthBuffer[lightIdx].m_depthStencilView );
				dd->m_context->ClearDepthStencilView( m_depthBuffer[lightIdx].m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

				getMatrices<true>( m_lightPos[lightIdx], make_float4(0,0,0), make_float4(1,0,0,0), XM_PI*60.f/180.f, 1.f, 
					0.1f, 50.f, &viewTr[lightIdx], &projTr[lightIdx] );
		
				dispatchRenderList( g_deviceData, &g_debugRenderObj, &viewTr[lightIdx], &projTr[lightIdx] );
			}

			{	//	run compute shader for accumulation
				dd->m_context->OMSetRenderTargets( 0, 0, 0 );

				cb.m_shadowIdx = lightIdx;

				Buffer<int> shadowBuffer;	shadowBuffer.m_srv = m_depthBuffer[lightIdx].m_srv;

				BufferInfo bInfo[] = { BufferInfo( &m_lightMergedBuffer ),
					BufferInfo( &shadowBuffer, true ) };

				Launcher launcher( m_deviceData, &m_copyShadowMapKernel );
				launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
				launcher.setConst( m_constBuffer, cb );
				launcher.launch1D( g_wWidth*g_wHeight, 64 );
			}
		}


		{	//	resolve in a shader
			{
				cb.m_shadowIdx = MAX_SHADOWS;
				for(int i=0; i<MAX_SHADOWS; i++)
				{
					viewTr[i] = XMMatrixMultiply( projTr[i], viewTr[i] );
				}
				m_lightMatrixBuffer.write( viewTr, MAX_SHADOWS );
				DeviceUtils::waitForCompletion( m_deviceData );
			}

			sw.start();

			Buffer<int> depthBuffer;	depthBuffer.m_srv = g_depthStencil.m_srv;
			Buffer<int> shadowBuffer;	shadowBuffer.m_srv = m_depthBuffer[0].m_srv;

			BufferInfo bInfo[] = { BufferInfo( &m_shadowAccumBuffer ), 
				BufferInfo( &depthBuffer, true ), 
				BufferInfo( &m_lightMergedBuffer, true ),
				BufferInfo( &m_lightMatrixBuffer, true ) };

			Launcher launcher( m_deviceData, &m_shadowAccmAllKernel );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( m_constBuffer, cb );
			launcher.launch2D( g_wWidth, g_wHeight, 8, 8 );

		}

		m_rtv->Release();
		m_dsv->Release();
	}
	sw.stop();

	{
		float t = sw.getMs();
		m_nTxtLines = 0;
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "%dlights, %3.2fms", MAX_SHADOWS, t);
	}

	resolve( &m_shadowAccumBuffer.m_srv );
//	resolveTexture( (void**)&m_colorRT.m_srv );
}

