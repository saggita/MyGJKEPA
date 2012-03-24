#include <GDemos/CullingDebugDemo.h>
#include <GDemos/CSCulling.h>
#include <common/Physics/BoxShape.h>


extern int g_wWidth;
extern int g_wHeight;

extern DeviceShaderDX11 g_bufferToRTPixelShader;
extern DeviceRenderTargetDX11 g_depthStencil;

extern XMMATRIX g_ViewTr;
extern XMMATRIX g_ProjectionTr;


CullingDebugDemo::CullingDebugDemo( const Device* device ) : PostProcessBaseDemo( device )
{
	{
		AdlAllocate();

		CSCulling::Config cfg;
		cfg.m_maxLights = MAX_LIGHTS;
		cfg.m_nTilesPerLights = 256;
		cfg.m_tileRes = 32;
		m_culling = new CSCulling( m_deviceData, cfg );
		m_culling->resize( g_wWidth, g_wHeight );
	}

	{	//	build kernel
		const char *option = "-I ..\\";
		KernelBuilder<TYPE_DX11> builder;
		builder.setFromFile( m_deviceData, "GDemos\\CullingDebugKernels", option, true );
		builder.createKernel("DisplayKernel", m_displayKernel );
		builder.createKernel("DisplayPixelBitsKernel", m_displayPixelKernel );
	}

	m_renderBuffer.allocate( m_deviceData, g_wWidth*g_wHeight );
	m_constBuffer.allocate( m_deviceData, 1, BufferBase::BUFFER_CONST );

	m_lightBuffer.allocate( m_deviceData, MAX_LIGHTS );
}

CullingDebugDemo::~CullingDebugDemo()
{
	{
		delete m_culling;
		AdlDeallocate();
	}
	KernelBuilder<TYPE_DX11>::deleteKernel( m_displayKernel );
	KernelBuilder<TYPE_DX11>::deleteKernel( m_displayPixelKernel );
}

void CullingDebugDemo::initDemo()
{
	{
		float4* lightBuffer = new float4[MAX_LIGHTS];

		float rad = 0.8f;
		if( MAX_LIGHTS > 1024 ) rad = 0.4f;
		if( MAX_LIGHTS > 2048 ) rad = 0.2f;
		float s = 12.f;
		for(int i=0; i<MAX_LIGHTS; i++) 
		{
			lightBuffer[i] = make_float4((i-MAX_LIGHTS/2)*rad*3.f, 0.f, 0.f, rad);
			lightBuffer[i].x = getRandom( -s, s );
			lightBuffer[i].z = getRandom( -s, s );
			lightBuffer[i].w = getRandom( rad*0.8f, rad*1.2f );
		}

		m_lightBuffer.write( lightBuffer, MAX_LIGHTS );
		DeviceUtils::waitForCompletion( m_deviceData );

		delete [] lightBuffer;
	}
}

void CullingDebugDemo::renderPre()
{
	prepareGBuffer();
}


void CullingDebugDemo::renderPost()
{
	DeviceDX11* dd = (DeviceDX11*)m_deviceData;
	dd->m_context->OMSetRenderTargets( 0, NULL, NULL );

	Buffer<float> depthBuffer; depthBuffer.m_srv = g_depthStencil.m_srv;

	Stopwatch sw( m_deviceData );

	sw.start();

	if( ENABLE_PIXEL_CULLING )
		m_culling->executePixelCulling( depthBuffer, m_lightBuffer, MAX_LIGHTS, g_ViewTr, g_ProjectionTr );
	else
		m_culling->execute( m_lightBuffer, MAX_LIGHTS, g_ViewTr, g_ProjectionTr );

	sw.stop();

	{
		float t = sw.getMs();
		m_nTxtLines = 0;
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "%dlights, %3.2fms", MAX_LIGHTS, t);
	}

	ConstData constBuffer;
	{
		constBuffer.m_view = g_ViewTr;
		constBuffer.m_projection = g_ProjectionTr;
		XMVECTOR v;
		constBuffer.m_projectionInv = XMMatrixInverse( &v, g_ProjectionTr );
		constBuffer.m_width = g_wWidth;
		constBuffer.m_height = g_wHeight;
		constBuffer.m_tileRes = m_culling->getTileRes();
	}

	if( !ENABLE_PIXEL_CULLING )
	{	//	run compute shader for debug display
		Buffer<int> colorBuffer; colorBuffer.m_srv = m_colorRT.m_srv;

		BufferInfo bInfo[] = { BufferInfo( &m_renderBuffer ),
			BufferInfo( m_culling->getLowerBoundBuffer(), true), BufferInfo( m_culling->getUpperBoundBuffer(), true), 
			BufferInfo( m_culling->getIndexBuffer(), true ), BufferInfo( &m_lightBuffer, true ), 
			BufferInfo( &colorBuffer, true ), BufferInfo( &depthBuffer, true ) };

		Launcher launcher( m_deviceData, &m_displayKernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( m_constBuffer, constBuffer );

		int2 nTiles = m_culling->getNTiles();
		launcher.launch1D( nTiles.x*nTiles.y*64, 64 );
	}
	else
	{
		Buffer<int> colorBuffer; colorBuffer.m_srv = m_colorRT.m_srv;

		BufferInfo bInfo[] = { BufferInfo( &m_renderBuffer ),
			BufferInfo( m_culling->getLowerBoundBuffer(), true), BufferInfo( m_culling->getUpperBoundBuffer(), true), 
			BufferInfo( m_culling->getIndexBuffer(), true ), BufferInfo( &m_lightBuffer, true ), 
			BufferInfo( &colorBuffer, true ), BufferInfo( &depthBuffer, true ), BufferInfo( m_culling->getPixelLightBitsBuffer(), true ) };

		Launcher launcher( m_deviceData, &m_displayPixelKernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( m_constBuffer, constBuffer );

		int2 nTiles = m_culling->getNTiles();
		launcher.launch1D( nTiles.x*nTiles.y*64, 64 );
	}


	resolve( &m_renderBuffer.m_srv );
//	resolve( (void**)&m_colorRT.m_srv );
}

