#include <GDemos/OcclusionCullingDemo.h>

#include <common/Physics/BoxShape.h>
#include <Common/Utils/ObjLoader.h>
#include <AdlPrimitives/Sort/RadixSort.h>
#include <AdlPrimitives/Search/BoundSearch.h>

#if defined(DX11RENDER)

#endif

#if defined(DX11RENDER)
extern int g_wWidth;
extern int g_wHeight;


int OcclusionCullingDemo::getLowResWidth()
{
	return (g_wWidth/LOW_RES_SCALE+1);
}

int OcclusionCullingDemo::getLowResHeight()
{
	return (g_wHeight/LOW_RES_SCALE+1);
}

int OcclusionCullingDemo::getNCellsX()
{
	return (getLowResWidth()+CELL_RES-1)/CELL_RES;
}

int OcclusionCullingDemo::getNCellsY()
{
	return (getLowResHeight()+CELL_RES-1)/CELL_RES;
}

OcclusionCullingDemo::OcclusionCullingDemo( const Device* deviceData )
	: Demo()
{
	INITIALIZE_DEVICE_DATA( deviceData );

	DeviceRenderTargetDX11::createRenderTarget( m_deviceData, g_wWidth, g_wHeight, m_colorRT );

	{	//	build kernel
		const char *option = "-I ..\\";
		KernelBuilder<(DeviceType)MyDeviceType> builder;
		builder.setFromFile( m_deviceData, "GDemos\\OcclusionCullingKernels", option, true );
		builder.createKernel("PostProcessKernel", m_kernel );
		builder.createKernel("ZReduceKernel", m_zReduceKernel );
		builder.createKernel("UpScaleKernel", m_upScaleKernel );
		builder.createKernel("QueryKernel", m_queryKernel );
		builder.createKernel("CountNOverlapCellsKernel", m_countNOverlapCellsKernel );
		builder.createKernel("FillSortDataKernel", m_fillSortDataKernel );
		builder.createKernel("CullByTileKernel", m_cullByTileKernel );
	}

	{
		ShaderUtilsDX11 builder( m_deviceData, "GDemos\\OcclusionCullingShader.hlsl" );
		builder.createPixelShader( "DepthPS", m_depthPShader );
	}

	m_buffer.allocate( m_deviceData, g_wWidth*g_wHeight );
	m_lowResDBuffer.allocate( m_deviceData, getLowResWidth()*getLowResHeight() );
	m_bbsBuffer.allocate( m_deviceData, MAX_BODIES );
	m_visibleFlagBuffer.allocate( m_deviceData, MAX_BODIES );
	m_cBuffer.allocate( m_deviceData, 1, BufferBase::BUFFER_CONST );
	m_nOverlappedCellBuffer.allocate( m_deviceData, MAX_BODIES );
	m_offsetBuffer.allocate( m_deviceData, MAX_BODIES );
	m_sortDataBuffer.allocate( m_deviceData, MAX_BODIES*MAX_OVL_PER_OBJ );
	m_lowerBoundBuffer.allocate( m_deviceData, MAX_CELL_X*MAX_CELL_Y );
	m_upperBoundBuffer.allocate( m_deviceData, MAX_CELL_X*MAX_CELL_Y );


	m_scanData = PrefixScan<(DeviceType)MyDeviceType>::allocate( m_deviceData, MAX_BODIES );


	m_pos = new float4[MAX_BODIES];
	m_shapes = new ShapeBase*[MAX_BODIES];
	m_visibleFlag = new u32[MAX_BODIES];


	m_maxDepth = 20.f;

	m_dispFlg0 = 0;
	m_nVisibleIdx = 0;


//	int nDevices = DUtils::getNumDevices();
}

OcclusionCullingDemo::~OcclusionCullingDemo()
{
	DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_colorRT );

	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_kernel );
	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_zReduceKernel );
	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_upScaleKernel );
	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_queryKernel );
	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_countNOverlapCellsKernel );
	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_fillSortDataKernel );
	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_cullByTileKernel );
	
	ShaderUtilsDX11::deleteShader( m_depthPShader );

	PrefixScan<(DeviceType)MyDeviceType>::deallocate( m_scanData );

	for(int i=0; i<MAX_BODIES; i++)
	{
		delete m_shapes[i];
	}

	delete [] m_pos;
	delete [] m_shapes;
	delete [] m_visibleFlag;

	//	finish ADL
	delete KernelManager::s_kManager;

	DESTROY_DEVICE_DATA;
}

void OcclusionCullingDemo::init()
{
	float extent = 1.5f;
	float e = 0.1f;
	for(int i=0; i<MAX_BODIES; i++)
	{
		ShapeBase* box;
		box = new BoxShape( make_float4(e,e,e,0) );
		m_shapes[i] = box;
		m_pos[i] = getRandom( make_float4(-extent, -0.5f*extent, -extent, 0.f), make_float4(extent, extent, 5.f*extent, 0.f) );
		
		float fov = 15.f*PI/180.f;
		float z = getRandom(-2.f*extent, 15.f*extent );
		float zz = z + 3.5f;
		m_pos[i].x = getRandom( -(zz*tan(fov)+extent), zz*tan(fov)+extent );
		m_pos[i].y = getRandom( -(zz*tan(fov)+extent), zz*tan(fov)+extent );
		m_pos[i].z = z;

		if( i==0 ) m_pos[i] = make_float4( 0.f, 0.f, 1.f );

		m_pos[i].w = sqrtf( e*e*3 );
	}

	m_bbsBuffer.write( m_pos, MAX_BODIES );
}

extern DeviceShaderDX11 g_defaultPixelShader;
extern DeviceShaderDX11 g_textureMapWColorPixelShader;
extern ID3D11SamplerState* g_defaultSampler;
extern DeviceShaderDX11 g_bufferToRTPixelShader;


extern XMMATRIX g_ViewTr;
extern XMMATRIX g_ProjectionTr;



void OcclusionCullingDemo::render()
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
		color = make_float4(0,1,0,1);

		if( i==0 ) color = make_float4(1.f,0.f,0.f, 1.f);

		if( m_visibleFlag[ i ] )
		{
			if( i==-908 )
			{
				u32 v =m_visibleFlag[i];
				u32 nx = v & 0xf;
				u32 ny = (v >> 8)&0xf;
				u32 nz = (v >> 16);//&0xf;
				u32 nw = 0;//(v >> 24)&0xf;

				adlDebugPrintf("%d, %d (%d:%d)\n", nx, ny, nz, nw);

				int a=0;
				a++;
			}

			if( m_visibleFlag[ i ] & 1 )
			{
				color.z = 1.f;
				color.y = 0.f;
			}
			else if( (m_visibleFlag[ i ] >> 1) & 1 )
			{
				color.z = 1.f;
				color.y = 1.f;
			}
			else
			{
				color = make_float4(1,0,0,1);
//				adlDebugPrintf("%d\n", i);
			}
		}

		for(int it=0; it<boxShape->getNumTris(); it++)
		{
			const int4& t = tris[it];

			idx[3*it+0] = it*3;
			idx[3*it+1] = it*3+1;
			idx[3*it+2] = it*3+2;

			v[3*it+0] = vtx[t.x];
			v[3*it+1] = vtx[t.y];
			v[3*it+2] = vtx[t.z];

			float4 tn = cross3( v[3*it+1] - v[3*it+0], v[3*it+2] - v[3*it+0] );

			tn = normalize3( tn );
			n[3*it+0] = tn;
			n[3*it+1] = tn;
			n[3*it+2] = tn;
		}

		int nTris = boxShape->getNumTris();
		int nVtx = boxShape->getNumVertex();


		pxDrawTriangleListTransformed( v, n, idx, nTris*3, nTris*3, color, pos, quat );

		delete [] v;
		delete [] idx;
		delete [] n;
	}
}

void OcclusionCullingDemo::renderPre()
{
//	if(1) return;

	m_globalPixelShader = g_defaultPixelShader;
	g_defaultPixelShader = m_depthPShader;

	DeviceDX11* dd = (DeviceDX11*)m_deviceData;

	//	swap render target using current depth stencil
	ID3D11RenderTargetView* pOrigRTV = NULL;
	ID3D11DepthStencilView* pOrigDSV = NULL;
	dd->m_context->OMGetRenderTargets( 1, &pOrigRTV, &pOrigDSV );

	float ClearColor[4] = { 0.f, 0.f, 0.f, m_maxDepth };

	ID3D11RenderTargetView* targets[] = { m_colorRT.m_renderTarget};

	dd->m_context->OMSetRenderTargets( 1, targets, pOrigDSV );
	dd->m_context->ClearRenderTargetView( m_colorRT.m_renderTarget, ClearColor );
	dd->m_context->ClearDepthStencilView( pOrigDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );
}

void OcclusionCullingDemo::renderPost()
{
//	if(1) return;
	ADLASSERT( LOW_RES_SCALE == 8 );
	ADLASSERT( getNCellsX()*getNCellsY() <= MAX_CELL_X*MAX_CELL_Y );

	SortData* ptr;
	u32* ptr0; u32* ptr1;


	u32 totalCount = 0;

	DeviceUtils::Config cfg;
	Device* deviceHost = DeviceUtils::allocate( TYPE_HOST, cfg );
	RadixSort<TYPE_HOST>::Data* dataS = RadixSort<TYPE_HOST>::allocate( deviceHost, MAX_BODIES*MAX_OVL_PER_OBJ );
	BoundSearch<TYPE_HOST>::Data* dataB = BoundSearch<TYPE_HOST>::allocate( deviceHost );



	Stopwatch dsw( m_deviceData );
	{	//	run CS
		CBuffer constData;
		{
			constData.m_view = g_ViewTr;
			constData.m_projection = g_ProjectionTr;
			constData.m_maxDepth = m_maxDepth;
			constData.m_fullWidth = g_wWidth;
			constData.m_lowResWidth = getLowResWidth();
			constData.m_lowResHeight = getLowResHeight();
			constData.m_nBodies = MAX_BODIES;
			constData.m_maxOvlPerObj = MAX_OVL_PER_OBJ;
		}

		BufferDX11<u32> csBuffer;
		{	//	Calculate low res Z
			csBuffer.m_srv = m_colorRT.m_srv;

			BufferInfo bInfo[] = { BufferInfo( &csBuffer, true ), BufferInfo( &m_buffer ), BufferInfo( &m_lowResDBuffer ) };

			Launcher launcher( m_deviceData, &m_zReduceKernel );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( m_cBuffer, constData );
			launcher.launch2D( g_wWidth, g_wHeight, LOW_RES_SCALE, LOW_RES_SCALE );
		}

		{
			dsw.start();
			
			if(1)
			{	//	Simple implementation
				BufferInfo bInfo[] = { BufferInfo( &m_buffer ), BufferInfo( &m_lowResDBuffer ), BufferInfo( &m_bbsBuffer, true ), BufferInfo( &m_visibleFlagBuffer ) };

				Launcher launcher( m_deviceData, &m_queryKernel );
				launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
				launcher.setConst( m_cBuffer, constData );
				launcher.launch1D( MAX_BODIES );
			}
			else
			{	//	Sophisticated implementation with tiles
				{	//	count n overlapped cells
					BufferInfo bInfo[] = { BufferInfo( &m_bbsBuffer, true ), 
						BufferInfo( &m_nOverlappedCellBuffer ), BufferInfo( &m_visibleFlagBuffer ) };
					Launcher launcher( m_deviceData, &m_countNOverlapCellsKernel );
					launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
					launcher.setConst( m_cBuffer, constData );
					launcher.launch1D( MAX_BODIES );
				}

				PrefixScan<(DeviceType)MyDeviceType>::execute( m_scanData, m_nOverlappedCellBuffer, m_offsetBuffer, MAX_BODIES, &totalCount );

				{	//	fill sort Data
					BufferInfo bInfo[] = { BufferInfo( &m_bbsBuffer, true ), BufferInfo( &m_offsetBuffer, true ), 
						BufferInfo( &m_sortDataBuffer ) };
					Launcher launcher( m_deviceData, &m_fillSortDataKernel );
					launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
					launcher.setConst( m_cBuffer, constData );
					launcher.launch1D( MAX_BODIES );
				}

				if(0)
				{
					HostBuffer<u32> host( deviceHost, MAX_BODIES );
					ptr0 = host.m_ptr;
					m_offsetBuffer.read( host.m_ptr, MAX_BODIES );
					DeviceUtils::waitForCompletion( m_deviceData );

					adlDebugPrintf( ">> %d\n", host[1750] );

				}

				{	//	sort by keys
					int nCells = getNCellsX()*getNCellsY();
					HostBuffer<SortData> sortData(deviceHost, totalCount);
					HostBuffer<u32> lowerBound(deviceHost, nCells );
					HostBuffer<u32> upperBound(deviceHost, nCells );

			ptr = sortData.m_ptr;
			ptr0 = lowerBound.m_ptr;
			ptr1 = upperBound.m_ptr;

					m_sortDataBuffer.read( sortData.m_ptr, totalCount );
					DeviceUtils::waitForCompletion( m_deviceData );


					RadixSort<TYPE_HOST>::execute( dataS, sortData, totalCount );
					
					BoundSearch<TYPE_HOST>::execute( dataB, sortData, totalCount, lowerBound, nCells, BoundSearchBase::BOUND_LOWER );
					BoundSearch<TYPE_HOST>::execute( dataB, sortData, totalCount, upperBound, nCells, BoundSearchBase::BOUND_UPPER );



					m_sortDataBuffer.write( sortData.m_ptr, totalCount );
					m_upperBoundBuffer.write( upperBound.m_ptr, nCells );
					m_lowerBoundBuffer.write( lowerBound.m_ptr, nCells );
					DeviceUtils::waitForCompletion( m_deviceData );
				}

				{	//	cull by tile
					BufferInfo bInfo[] = { BufferInfo( &m_lowerBoundBuffer, true ), BufferInfo( &m_upperBoundBuffer, true ), BufferInfo( &m_sortDataBuffer, true ), BufferInfo( &m_bbsBuffer, true ), BufferInfo( &m_lowResDBuffer, true ),
						BufferInfo( &m_visibleFlagBuffer ) };

					Launcher launcher( m_deviceData, &m_cullByTileKernel );
					launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
					launcher.setConst( m_cBuffer, constData );
					launcher.launch2D( getNCellsX()*CELL_RES, getNCellsY()*CELL_RES, CELL_RES, CELL_RES );
				}
			}

			dsw.stop();

			{	//	read back
				m_visibleFlagBuffer.read( m_visibleFlag, MAX_BODIES );
				DeviceUtils::waitForCompletion( m_deviceData );
				m_nVisibleIdx = 0;
				for(int i=0; i<MAX_BODIES; i++)
				{
					m_nVisibleIdx += (m_visibleFlag[i])? 1:0;
				}

				ADLASSERT( m_nVisibleIdx <= MAX_BODIES );
			}
		}

		if(m_dispFlg0%2 == 1)
		{	//	debug purpose
			BufferInfo bInfo[] = { BufferInfo( &csBuffer, true ), BufferInfo( &m_buffer ), BufferInfo( &m_lowResDBuffer ) };

			Launcher launcher( m_deviceData, &m_upScaleKernel );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( m_cBuffer, constData );
			launcher.launch2D( g_wWidth, g_wHeight, LOW_RES_SCALE, LOW_RES_SCALE );
		}
		else
		{	//	for debug
			BufferInfo bInfo[] = { BufferInfo( &csBuffer, true ), BufferInfo( &m_buffer ), BufferInfo( &m_lowResDBuffer ) };

			Launcher launcher( m_deviceData, &m_kernel );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( m_cBuffer, constData );
			launcher.launch2D( g_wWidth, g_wHeight, LOW_RES_SCALE, LOW_RES_SCALE );
		}
	}


	DeviceDX11* dd = (DeviceDX11*)m_deviceData;

	ID3D11RenderTargetView* pOrigRTV = NULL;
	ID3D11DepthStencilView* pOrigDSV = NULL;
	dd->m_context->OMGetRenderTargets( 1, &pOrigRTV, &pOrigDSV );

	//	release for the renderPre
	pOrigRTV->Release();
	pOrigDSV->Release();

	{
		float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f } ; // red, green, blue, alpha
		ID3D11RenderTargetView* aRTViews[ 1 ] = { pOrigRTV };
		dd->m_context->OMSetRenderTargets( 1, aRTViews, pOrigDSV );
		dd->m_context->ClearDepthStencilView( pOrigDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );
		dd->m_context->ClearRenderTargetView( pOrigRTV, ClearColor );

//		dd->m_context->PSSetShaderResources( 0, 1, &m_buffer.m_srv );
		dd->m_context->PSSetShaderResources( 0, 1, ((BufferDX11<float4>*)&m_buffer)->getSRVPtr() );

		//	render to screen
		renderFullQuad( m_deviceData, &g_bufferToRTPixelShader, 
			make_float4((float)g_wWidth, (float)g_wHeight, 0,0 ) );

		ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
		dd->m_context->PSSetShaderResources( 0, 1, ppSRVNULL );
	}

	pOrigRTV->Release();
	pOrigDSV->Release();

	g_defaultPixelShader = m_globalPixelShader;

	{
		m_nTxtLines = 0;
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "Visible: %3.2f %%[%3.2fms]", 100.f*m_nVisibleIdx/(float)MAX_BODIES,
			dsw.getMs());
	}

	BoundSearch<TYPE_HOST>::deallocate( dataB );
	RadixSort<TYPE_HOST>::deallocate( dataS );
	DeviceUtils::deallocate( deviceHost );

}

void OcclusionCullingDemo::keyListener(unsigned char key)
{
	switch( key )
	{
	case 'M':
	case 'm':
		m_dispFlg0 ++;
		break;
	default:
		break;
	};
}



#endif
