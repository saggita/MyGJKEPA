#include <GDemos/SimpleDeferredDemo.h>
#include <Common/Physics/SimpleConvexShape.h>
#include <common/Physics/BoxShape.h>
#include <Common/Utils/ObjLoader.h>



#if defined(DX11RENDER)

#endif

#if defined(DX11RENDER)
extern int g_wWidth;
extern int g_wHeight;

SimpleDeferredDemo::SimpleDeferredDemo(const Device* deviceData)
	: Demo()
{
//	m_enablePostEffect = 1;

	INITIALIZE_DEVICE_DATA( deviceData );

	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_colorRT );
	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_posRT ); 
	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_normalRT ); 

	{	//	build kernel
		const char *option = "-I ..\\";
		KernelBuilder<(DeviceType)MyDeviceType> builder;
		builder.setFromFile( m_deviceData, "GDemos\\SimpleDeferredDemoKernel", option, true );
		builder.createKernel("PostProcessKernel", m_kernel );

		builder.createKernel("ClearLightIdxKernel", m_clearLightIdxKernel );
		builder.createKernel("BuildLightIdxKernel", m_buildLightIdxKernel );

	}

	m_buffer.allocate( m_deviceData, g_wWidth*g_wHeight );
	m_lightPosBuffer.allocate( m_deviceData, MAX_LIGHTS );
	m_lightColorBuffer.allocate( m_deviceData, MAX_LIGHTS );

	{
		ADLASSERT( MAX_LIGHTS_PER_TILE%32 == 0 );

		int nClusterX = calcNumTiles(g_wWidth, CLUSTER_SIZE);
		int nClusterY = calcNumTiles(g_wHeight, CLUSTER_SIZE);

		m_tileBuffer.allocate( m_deviceData, MAX_LIGHTS_PER_TILE_IN_32B*nClusterX*nClusterY );
	}


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

SimpleDeferredDemo::~SimpleDeferredDemo()
{
	for(int i=0; i<MAX_BODIES; i++)
	{
		delete m_shapes[i];
	}

	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_colorRT );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_posRT );
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_normalRT );

	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_kernel );
	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_clearLightIdxKernel );
	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_buildLightIdxKernel );
	
	ShaderUtilsDX11::deleteShader( m_gpVShader );
	ShaderUtilsDX11::deleteShader( m_gpPShader );

	DESTROY_DEVICE_DATA;
}

void SimpleDeferredDemo::init()
{
	{
		float floorExtent = 6.0f;
		ObjLoader* objLoader = new ObjLoader("../Resources/Obj/aQuad.obj");
		{
			Aabb aabb;
			aabb.m_min = make_float4(-floorExtent);
			aabb.m_max = make_float4( floorExtent);
			aabb.m_min.y = 0.f;
			aabb.m_max.y = 0.1f;
			ObjLoader::scale( aabb, objLoader->getVertexBuffer(), objLoader->getNumVertices() );
		}
		SimpleConvexShape* cvxShape = new SimpleConvexShape;
		cvxShape->set( make_float4(-0.4f), qtGetIdentity(), objLoader->getVertexBuffer(), objLoader->getNumVertices(),
			objLoader->getFaceIndexBuffer(), objLoader->getNumTriangles() );

		m_shapes[0] = cvxShape;
		m_pos[0] = make_float4(-0.f);

		delete objLoader;
	}

	float e = 0.3f;
	for(int i=1; i<MAX_BODIES; i++)
	{
		int ii = (i-MAX_BODIES/2);
		ShapeBase* box;
		box = new BoxShape( make_float4(e,e,e,0) );
		m_shapes[i] = box;
		m_pos[i] = make_float4(ii*e*3.f, e*1.5f, e*ii*3);
	}



	{
		float4* p = new float4[MAX_LIGHTS];
		float4* c = new float4[MAX_LIGHTS];

		float x = 5.f;
		float y = 0.7f;
		float fallOffMul = 1.1f;

		for(int i=0; i<MAX_LIGHTS; i++)
		{
			p[i] = make_float4( getRandom(-x,x), getRandom(0.f, y), getRandom(-x,x), getRandom(x*(fallOffMul-1.f), fallOffMul) );
			c[i] = make_float4( getRandom(0.f,1.f), getRandom(0.f,1.f), getRandom(0.f,1.f) );
//			p[i].y = (p[i].x+x)/5.f*0.5f;
//			p[i].w = p[i].y*2.f;
		}

//		p[0] = make_float4(0,fallOffMul*0.5f,x,fallOffMul);
//		c[0] = make_float4(1,0,0,1);

		m_lightPosBuffer.write( p, MAX_LIGHTS );
		m_lightColorBuffer.write( c, MAX_LIGHTS );
		DeviceUtils::waitForCompletion( m_deviceData );

		delete [] p;
		delete [] c;
	}

	m_firstFrame = true;
}

void SimpleDeferredDemo::render()
{
	if( m_firstFrame )
	{
	for(int i=0; i<MAX_BODIES; i++)
	{
		const float4& pos = m_pos[i];
		const Quaternion& quat = qtGetIdentity();

		ShapeBase* boxShape = m_shapes[i];

		const float4* vtx = boxShape->getVertexBuffer();
		const int4* tris = boxShape->getTriangleBuffer();

		float4* v = new float4[boxShape->getNumTris()*3];
		u32* idx = new u32[boxShape->getNumTris()*3];
		float4* n = new float4[boxShape->getNumTris()*3];

		const float4 colors[] = { make_float4(0,1,1,1), make_float4(1,0,1,1), make_float4(1,1,0,1),
			make_float4(0,0,1,1), make_float4(0,1,0,1), make_float4(1,0,0,1) };

		float c = 0.4f;
		float4 color = make_float4(0.8f-c,0.8f-c,0.8f,1.f)*1.8f;
		color = make_float4(0.5f,1,0.5f,1);

		if( i==0 ) color = make_float4(1.f,1.f,1.f, 1.f);

		for(int it=0; it<boxShape->getNumTris(); it++)
		{
			const int4& t = tris[it];

			idx[3*it+0] = it*3;
			idx[3*it+1] = it*3+1;
			idx[3*it+2] = it*3+2;

			v[3*it+0] = vtx[t.x];
			v[3*it+1] = vtx[t.y];
			v[3*it+2] = vtx[t.z];
			if( i==0 ) swap2(v[3*it+1], v[3*it+2]);

			float4 tn = cross3( v[3*it+1] - v[3*it+0], v[3*it+2] - v[3*it+0] );

			tn = normalize3( tn );
			n[3*it+0] = tn;
			n[3*it+1] = tn;
			n[3*it+2] = tn;
		}

		int nTris = boxShape->getNumTris();
		int nVtx = boxShape->getNumVertex();


//		pxReleaseDrawTriangleListTransformed( v, n, idx, nTris*3, nTris*3, color, pos, quat );
		pxDrawTriangleListTransformed( v, n, idx, nTris*3, nTris*3, color, pos, quat );

		delete [] v;
		delete [] idx;
		delete [] n;
	}
//	m_firstFrame = false;
	}
}

void SimpleDeferredDemo::renderPre()
{
//	if(1) return;

	DeviceDX11* dd = (DeviceDX11*)m_deviceData;

	//	swap render target using current depth stencil
	ID3D11RenderTargetView* pOrigRTV = NULL;
	ID3D11DepthStencilView* pOrigDSV = NULL;
	dd->m_context->OMGetRenderTargets( 1, &pOrigRTV, &pOrigDSV );

	float ClearColor[4] = { 0.f, 0.f, 0.f, 0.0f };

	ID3D11RenderTargetView* targets[] = { m_colorRT.m_renderTarget, m_posRT.m_renderTarget, m_normalRT.m_renderTarget };

	dd->m_context->OMSetRenderTargets( 3, targets, pOrigDSV );
	dd->m_context->ClearRenderTargetView( m_colorRT.m_renderTarget, ClearColor );
	dd->m_context->ClearRenderTargetView( m_posRT.m_renderTarget, ClearColor );
	dd->m_context->ClearRenderTargetView( m_normalRT.m_renderTarget, ClearColor );
	dd->m_context->ClearDepthStencilView( pOrigDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	g_defaultVertexShader = m_gpVShader;
	g_defaultPixelShader = m_gpPShader;
}


extern DeviceShaderDX11 g_defaultPixelShader;
extern ID3D11SamplerState* g_defaultSampler;
extern BufferDX11<ConstantBuffer>	 g_constBuffer;
extern DeviceShaderDX11 g_bufferToRTPixelShader;

extern XMMATRIX g_ViewTr;
extern XMMATRIX g_ProjectionTr;
/*
struct ConstantBuffer
{
	XMMATRIX m_world;
	XMMATRIX m_view;
	XMMATRIX m_projection;
};
*/

void SimpleDeferredDemo::renderPost()
{
//	if(1) return;

	ADLASSERT( TILE_SIZE <= 16 );

	int nClusterX = calcNumTiles(g_wWidth, CLUSTER_SIZE);//max2( 1, (g_wWidth/CLUSTER_SIZE)+(!(g_wWidth%CLUSTER_SIZE)?0:1) );
	int nClusterY = calcNumTiles(g_wHeight, CLUSTER_SIZE);//max2( 1, (g_wHeight/CLUSTER_SIZE)+(!(g_wHeight%CLUSTER_SIZE)?0:1) );

	//	todo. define own constant buffer
	ConstantBuffer cb;
	{
		cb.m_world = XMMatrixIdentity();
		cb.m_view = g_ViewTr;
		cb.m_projection = g_ProjectionTr;
	}
	{	//	clear lightIdxBuffer
		BufferInfo bInfo[] = { BufferInfo( &m_tileBuffer ) };

		Launcher launcher( m_deviceData, &m_clearLightIdxKernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
//		launcher.pushBackRW( m_tileBuffer );
		launcher.launch1D( nClusterX*nClusterY*MAX_LIGHTS_PER_TILE_IN_32B );
	}

	{	//	set lightIdxBuffer
		BufferInfo bInfo[] = { BufferInfo( &m_lightPosBuffer, true ), BufferInfo( &m_lightColorBuffer, true ), 
			BufferInfo( &m_tileBuffer ) };

		Launcher launcher( m_deviceData, &m_buildLightIdxKernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
//		launcher.pushBackR( m_lightPosBuffer );
//		launcher.pushBackR( m_lightColorBuffer );
//		launcher.pushBackRW( m_tileBuffer );
		launcher.setConst( g_constBuffer, cb );
		launcher.launch1D( 64 );
	}

	Stopwatch dsw( m_deviceData );
	dsw.start();
	{	//	run CS
		BufferDX11<int> cBuffer;
		cBuffer.m_srv = m_colorRT.m_srv;

		BufferDX11<int> pBuffer;
		pBuffer.m_srv = m_posRT.m_srv;

		BufferDX11<int> nBuffer;
		nBuffer.m_srv = m_normalRT.m_srv;

		BufferInfo bInfo[] = { BufferInfo( &m_lightPosBuffer, true ), BufferInfo( &m_lightColorBuffer, true ), BufferInfo( &cBuffer, true ),
			BufferInfo( &pBuffer, true ), BufferInfo( &nBuffer, true ), BufferInfo( &m_tileBuffer, true ),
			BufferInfo( &m_buffer ) };

		Launcher launcher( m_deviceData, &m_kernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
//		launcher.pushBackR( m_lightPosBuffer );
//		launcher.pushBackR( m_lightColorBuffer );
//		launcher.pushBackR( cBuffer );
//		launcher.pushBackR( pBuffer );
//		launcher.pushBackR( nBuffer );
//		launcher.pushBackR( m_tileBuffer );
//		launcher.pushBackRW( m_buffer );
		launcher.setConst( g_constBuffer, cb );
		launcher.launch2D( nClusterX*TILE_SIZE, nClusterY*TILE_SIZE, TILE_SIZE, TILE_SIZE );
	}
	dsw.stop();

	{
		m_nTxtLines = 0;
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "%3.3fms", dsw.getMs());
	}

	//

	DeviceDX11* dd = (DeviceDX11*)m_deviceData;

	ID3D11RenderTargetView* pOrigRTV = NULL;
	ID3D11DepthStencilView* pOrigDSV = NULL;
	dd->m_context->OMGetRenderTargets( 1, &pOrigRTV, &pOrigDSV );

	//	release for the renderPre
	pOrigRTV->Release();
	pOrigDSV->Release();

	{
		float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f };
		ID3D11RenderTargetView* aRTViews[ 1 ] = { pOrigRTV };
		dd->m_context->OMSetRenderTargets( 1, aRTViews, pOrigDSV );
		dd->m_context->ClearDepthStencilView( pOrigDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );
		dd->m_context->ClearRenderTargetView( pOrigRTV, ClearColor );

		dd->m_context->PSSetShaderResources( 0, 1, ((BufferDX11<float4>*)&m_buffer)->getSRVPtr() );

		//	render to screen
		renderFullQuad( m_deviceData, &g_bufferToRTPixelShader, make_float4((float)g_wWidth, (float)g_wHeight, 0,0 ) );

		ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
		dd->m_context->PSSetShaderResources( 0, 1, ppSRVNULL );
	}

	pOrigRTV->Release();
	pOrigDSV->Release();

	g_defaultVertexShader = m_gVShader;
	g_defaultPixelShader = m_gPShader;
}

int SimpleDeferredDemo::calcNumTiles(int size, int clusterSize)
{
	return max2( 1, (size/clusterSize)+(!(size%clusterSize)?0:1) );
}
#endif