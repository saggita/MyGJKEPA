#include <GDemos/CSCulling.h>


CSCulling::CSCulling( const Device* device, const Config& cfg )
	: m_cfg(cfg), m_lowerBoundBuffer(0), m_upperBoundBuffer(0), m_indexBuffer(0), 
	m_visibleFlagBuffer(0), m_nOverlappedCellBuffer(0), m_offsetBuffer(0), m_pixelLightBitsBuffer(0),
	
	m_scanData(0), m_searchData(0), m_sortData(0), m_fillData(0)
{
	ADLASSERT( device );
	ADLASSERT( device->m_type == TYPE_DX11 );
	const DeviceDX11* dd = (const DeviceDX11*)device;
	m_device.m_context = dd->m_context;
	m_device.m_device = dd->m_device;


	m_size = make_int2(-1,-1);

	{
		KernelBuilder<TYPE_DX11> builder;
		builder.setFromFile( &m_device, "GDemos\\CSCullingKernels", 0, true );
		builder.createKernel("CountNOverlapCellsKernel", m_countNOverlapCellsKernel );
		builder.createKernel("FillSortDataKernel", m_fillSortDataKernel );
		builder.createKernel("PixelLightCullingKernel", m_pixelLightCullingKernel );
	}

	m_constBuffer[0] = m_constBuffer[1] = m_constBuffer[2] = m_constBuffer[3] = 0;
}

CSCulling::~CSCulling()
{
	deallocateBuffers();

	KernelBuilder<TYPE_DX11>::deleteKernel( m_countNOverlapCellsKernel );
	KernelBuilder<TYPE_DX11>::deleteKernel( m_fillSortDataKernel );
	KernelBuilder<TYPE_DX11>::deleteKernel( m_pixelLightCullingKernel );
}

void CSCulling::resize(int width, int height)
{
	m_size = make_int2( width, height );

	deallocateBuffers();
	allocateBuffers( width, height );
}

void CSCulling::execute(const Buffer<float4>& spheres, int nSpheres, const XMMATRIX& view, const XMMATRIX& projection)
{
	ADLASSERT( m_size.x != -1 && m_size.y != -1 );
	ADLASSERT( m_lowerBoundBuffer && m_upperBoundBuffer && m_indexBuffer );
	ADLASSERT( nSpheres <= m_cfg.m_maxLights );

	u32 totalCount = 0;

	CBuffer constData;
	{
		constData.m_view = view;
		constData.m_projection = projection;
		XMVECTOR v;
		constData.m_projectionInv = XMMatrixInverse( &v, projection );
		constData.m_width = m_size.x;
		constData.m_height = m_size.y;
		constData.m_nBodies = nSpheres;
		constData.m_maxOvlPerObj = 0xffff;
		constData.m_tileRes = m_cfg.m_tileRes;
	}
	
	DeviceUtils::Config cfg;
	Device* deviceHost = DeviceUtils::allocate( TYPE_HOST, cfg );
	{	//	count # overlapping cells
		BufferInfo bInfo[] = { BufferInfo( (Buffer<float4>*)&spheres, true ), 
			BufferInfo( m_nOverlappedCellBuffer ), BufferInfo( m_visibleFlagBuffer ) };
		Launcher launcher( &m_device, &m_countNOverlapCellsKernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( *m_constBuffer[0], constData );
		launcher.launch1D( nSpheres );
	}

	PrefixScan<TYPE_DX11>::execute( m_scanData, *m_nOverlappedCellBuffer, *m_offsetBuffer, nSpheres, &totalCount );

	{
		int2 tileRes = getNTiles();
		int nCells = tileRes.x*tileRes.y;

		Fill<TYPE_DX11>::execute( m_fillData, (Buffer<int>&)*m_lowerBoundBuffer, 0, nCells );
		Fill<TYPE_DX11>::execute( m_fillData, (Buffer<int>&)*m_upperBoundBuffer, 0, nCells );
	}

	if( totalCount==0 )
		return;

	{	//	fill sort data
		BufferInfo bInfo[] = { BufferInfo( (Buffer<float4>*)&spheres, true ), BufferInfo( m_offsetBuffer, true ), 
			BufferInfo( m_indexBuffer ) };
		Launcher launcher( &m_device, &m_fillSortDataKernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( *m_constBuffer[1], constData );
		launcher.launch1D( nSpheres );
	}

	if(1)
	{
		int2 tileRes = getNTiles();
		int nCells = tileRes.x*tileRes.y;

		int sortSize = NEXTMULTIPLEOF( totalCount, 512 );

		if( sortSize != totalCount )
			Fill<TYPE_DX11>::execute( m_fillData, (Buffer<int2>&)*m_indexBuffer, make_int2(0xffffffff, 0), sortSize-totalCount, totalCount );

		int sortBits = 32;
		if( nCells < (1<<20) ) sortBits = 20;
		if( nCells < (1<<16) ) sortBits = 16;
		if( nCells < (1<<12) ) sortBits = 12;
		if( nCells < (1<<8) ) sortBits = 8;

		RadixSort<TYPE_DX11>::execute( m_sortData, *m_indexBuffer, sortSize, sortBits );

		BoundSearch<TYPE_DX11>::execute( m_searchData, *m_indexBuffer, totalCount, *m_lowerBoundBuffer, nCells, BoundSearchBase::BOUND_LOWER );
		BoundSearch<TYPE_DX11>::execute( m_searchData, *m_indexBuffer, totalCount, *m_upperBoundBuffer, nCells, BoundSearchBase::BOUND_UPPER );

		if(0)
		{
			u32* lhost = new u32[nCells];
			u32* hhost = new u32[nCells];

			m_lowerBoundBuffer->read( lhost, nCells );
			m_upperBoundBuffer->read( hhost, nCells );
			DeviceUtils::waitForCompletion( &m_device );

			int a=0;
			a++;

			delete [] lhost;
			delete [] hhost;
		}
	}
	else if(0)
	{	//	sort by keys(tile), and get bound
		DeviceUtils::waitForCompletion( &m_device );

		RadixSort<TYPE_HOST>::Data* dataS = RadixSort<TYPE_HOST>::allocate( deviceHost, totalCount );
		{
			int2 tileRes = getNTiles();
			int nCells = tileRes.x*tileRes.y;
			HostBuffer<SortData> sortData(deviceHost, totalCount);

			m_indexBuffer->read( sortData.m_ptr, totalCount );
			DeviceUtils::waitForCompletion( &m_device );


			RadixSort<TYPE_HOST>::execute( dataS, sortData, totalCount );
					
			m_indexBuffer->write( sortData.m_ptr, totalCount );
			DeviceUtils::waitForCompletion( &m_device );

			BoundSearch<TYPE_DX11>::execute( m_searchData, *m_indexBuffer, totalCount, *m_lowerBoundBuffer, nCells, BoundSearchBase::BOUND_LOWER );
			BoundSearch<TYPE_DX11>::execute( m_searchData, *m_indexBuffer, totalCount, *m_upperBoundBuffer, nCells, BoundSearchBase::BOUND_UPPER );
		}

		RadixSort<TYPE_HOST>::deallocate( dataS );
	}
	else
	{	//	sort by keys(tile), and get bound
		DeviceUtils::waitForCompletion( &m_device );

		RadixSort<TYPE_HOST>::Data* dataS = RadixSort<TYPE_HOST>::allocate( deviceHost, totalCount );
		BoundSearch<TYPE_HOST>::Data* dataB = BoundSearch<TYPE_HOST>::allocate( deviceHost );

		{
			int2 tileRes = getNTiles();
			int nCells = tileRes.x*tileRes.y;
			HostBuffer<SortData> sortData(deviceHost, totalCount);
			HostBuffer<u32> lowerBound(deviceHost, nCells );
			HostBuffer<u32> upperBound(deviceHost, nCells );

			m_indexBuffer->read( sortData.m_ptr, totalCount );
			m_lowerBoundBuffer->read( lowerBound.m_ptr, nCells );
			m_upperBoundBuffer->read( upperBound.m_ptr, nCells );
			DeviceUtils::waitForCompletion( &m_device );


			RadixSort<TYPE_HOST>::execute( dataS, sortData, totalCount );
					
			BoundSearch<TYPE_HOST>::execute( dataB, sortData, totalCount, lowerBound, nCells, BoundSearchBase::BOUND_LOWER );
			BoundSearch<TYPE_HOST>::execute( dataB, sortData, totalCount, upperBound, nCells, BoundSearchBase::BOUND_UPPER );


			m_indexBuffer->write( sortData.m_ptr, totalCount );
			m_upperBoundBuffer->write( upperBound.m_ptr, nCells );
			m_lowerBoundBuffer->write( lowerBound.m_ptr, nCells );
			DeviceUtils::waitForCompletion( &m_device );
		}

		BoundSearch<TYPE_HOST>::deallocate( dataB );
		RadixSort<TYPE_HOST>::deallocate( dataS );
	}
	DeviceUtils::deallocate( deviceHost );
}

void CSCulling::executePixelCulling(const Buffer<float>& depthBuffer, const Buffer<float4>& spheres, int nSpheres, 
			const XMMATRIX& view, const XMMATRIX& projection)
{
	execute( spheres, nSpheres, view, projection );

	CBuffer constData;
	{
		constData.m_view = view;
		constData.m_projection = projection;
		XMVECTOR v;
		constData.m_projectionInv = XMMatrixInverse( &v, projection );
		constData.m_width = m_size.x;
		constData.m_height = m_size.y;
		constData.m_nBodies = nSpheres;
		constData.m_maxOvlPerObj = 0xffff;
		constData.m_tileRes = m_cfg.m_tileRes;
	}

	{	//	set pixelLightBitsBuffer
		BufferInfo bInfo[] = { BufferInfo( m_pixelLightBitsBuffer ),
			BufferInfo( getLowerBoundBuffer(), true ), BufferInfo( getUpperBoundBuffer(), true ),
			BufferInfo( getIndexBuffer(), true ), 
			BufferInfo( (Buffer<float4>*)&spheres, true ), BufferInfo( (Buffer<float>*)&depthBuffer, true ) };

		Launcher launcher( &m_device, &m_pixelLightCullingKernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( *m_constBuffer[0], constData );
		int2 nTiles = getNTiles();
		launcher.launch1D( nTiles.x*nTiles.y*64, 64 );
	}
}

int2 CSCulling::getNTiles()
{
	return make_int2( m_size.x/m_cfg.m_tileRes+1, m_size.y/m_cfg.m_tileRes+1 );
}

void CSCulling::allocateBuffers(int width, int height)
{
	int2 tileRes = getNTiles();
	int nTiles = tileRes.x*tileRes.y;

	m_lowerBoundBuffer = new Buffer<u32>( &m_device, nTiles );
	m_upperBoundBuffer = new Buffer<u32>( &m_device, nTiles );
	m_indexBuffer = new Buffer<SortData>( &m_device, nTiles*m_cfg.m_nTilesPerLights );
	m_visibleFlagBuffer = new Buffer<u32>( &m_device, m_cfg.m_maxLights );
	m_nOverlappedCellBuffer = new Buffer<u32>( &m_device, m_cfg.m_maxLights );
	m_offsetBuffer = new Buffer<u32>( &m_device, m_cfg.m_maxLights );
	m_pixelLightBitsBuffer = new Buffer<u32>( &m_device, m_size.x*m_size.y );
	for(int i=0; i<4; i++)
	{
		m_constBuffer[i] = new Buffer<CBuffer>( &m_device, 1, BufferBase::BUFFER_CONST );
	}

	m_scanData = PrefixScan<TYPE_DX11>::allocate( &m_device, m_cfg.m_maxLights );
	m_searchData = BoundSearch<TYPE_DX11>::allocate( &m_device );
	m_sortData = RadixSort<TYPE_DX11>::allocate( &m_device, m_cfg.m_maxLights*m_cfg.m_nTilesPerLights, RadixSortBase::SORT_STANDARD );
	m_fillData = Fill<TYPE_DX11>::allocate( &m_device );
}

void CSCulling::deallocateBuffers()
{
	if( m_lowerBoundBuffer ) delete m_lowerBoundBuffer;
	if( m_upperBoundBuffer ) delete m_upperBoundBuffer;
	if( m_indexBuffer ) delete m_indexBuffer;
	if( m_visibleFlagBuffer ) delete m_visibleFlagBuffer;
	if( m_nOverlappedCellBuffer ) delete m_nOverlappedCellBuffer;
	if( m_offsetBuffer ) delete m_offsetBuffer;
	if( m_pixelLightBitsBuffer ) delete m_pixelLightBitsBuffer;
	for(int i=0; i<4; i++)
	{
		if( m_constBuffer[i] ) delete m_constBuffer[i];
		m_constBuffer[i] = 0;
	}

	if( m_scanData ) PrefixScan<TYPE_DX11>::deallocate( m_scanData );
	if( m_searchData ) BoundSearch<TYPE_DX11>::deallocate( m_searchData );
	if( m_sortData ) RadixSort<TYPE_DX11>::deallocate( m_sortData );
	if( m_fillData ) Fill<TYPE_DX11>::deallocate( m_fillData );

	m_lowerBoundBuffer = 0;
	m_upperBoundBuffer = 0;
	m_indexBuffer = 0;
	m_visibleFlagBuffer = 0;
	m_nOverlappedCellBuffer = 0;
	m_offsetBuffer = 0;
	m_pixelLightBitsBuffer = 0;

	m_scanData = 0;
	m_searchData = 0;
	m_sortData = 0;
	m_fillData = 0;
}

