#include <Demos/AdlPrimitivesDemo.h>

AdlPrimitivesDemo::AdlPrimitivesDemo( const Device* device ) : Demo()
{
	INITIALIZE_DEVICE_DATA( device );
	AdlAllocate();
}

AdlPrimitivesDemo::~AdlPrimitivesDemo()
{
	AdlDeallocate();
	DESTROY_DEVICE_DATA;
}

void AdlPrimitivesDemo::reset()
{

}

void AdlPrimitivesDemo::step(float dt)
{

}

void AdlPrimitivesDemo::render()
{
	int size = 1024*256;
//	int size = 1024*64;
	size = NEXTMULTIPLEOF( size, 512 );

	int* host1 = new int[size];
	int2* host2 = new int2[size];
	int4* host4 = new int4[size];
	for(int i=0; i<size; i++) { host1[i] = getRandom(0,0xffff); host2[i] = make_int2( host1[i], i ); host4[i] = make_int4( host2[i].x, host2[i].y, host2[i].x, host2[i].y ); }
	Buffer<int> buf1( m_deviceData, size );
	Buffer<int2> buf2( m_deviceData, size );
	Buffer<int4> buf4( m_deviceData, size );
	buf1.write( host1, size );
	buf2.write( host2, size );
	buf4.write( host4, size );

	Stopwatch sw( m_deviceData );

	m_nTxtLines = 0;
	sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "%d elems", size);
//	testSort( (Buffer<SortData>&)buf2, size, sw );
	testFill1( buf1, size, sw );
	testFill2( buf2, size, sw );
	testFill4( buf4, size, sw );

	test( buf2, size, sw );

	delete [] host1;
	delete [] host2;
	delete [] host4;
}

void AdlPrimitivesDemo::testSort( Buffer<SortData>& buf, int size, Stopwatch& sw )
{
	MySort::Data* sortData = MySort::allocate( m_deviceData, size, RadixSortBase::SORT_ADVANCED );
	
	sw.start();

	MySort::execute( sortData, buf, size );

	sw.stop();

	MySort::deallocate( sortData );

	{
		m_nTxtLines = 0;
		float t = sw.getMs();
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "%d: %3.2fms, %3.2fMKeys/s", size, t, size/t/1000);		
	}
}

void AdlPrimitivesDemo::testFill1( Buffer<int>& buf, int size, Stopwatch& sw )
{
	MyFill::Data* sortData = MyFill::allocate( m_deviceData );
	
	sw.start();

	MyFill::execute( sortData, buf, 12, size );

	sw.stop();

	MyFill::deallocate( sortData );

	{
		float t = sw.getMs();
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "Fill int: %3.2fGB/s (%3.2fms)", size/t/1000/1000*4, t);		
	}
}

void AdlPrimitivesDemo::testFill2( Buffer<int2>& buf, int size, Stopwatch& sw )
{
	MyFill::Data* sortData = MyFill::allocate( m_deviceData );
	
	sw.start();

	MyFill::execute( sortData, buf, make_int2(12, 13), size );

	sw.stop();

	MyFill::deallocate( sortData );

	{
		float t = sw.getMs();
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "Fill int2: %3.2fGB/s (%3.2fms)", size/t/1000/1000*8, t);		
	}
}

void AdlPrimitivesDemo::testFill4( Buffer<int4>& buf, int size, Stopwatch& sw )
{
	MyFill::Data* sortData = MyFill::allocate( m_deviceData );
	
	sw.start();

	MyFill::execute( sortData, buf, make_int4(12, 13, 1, 2), size );

	sw.stop();

	MyFill::deallocate( sortData );

	{
		float t = sw.getMs();
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "Fill int4: %3.2fGB/s (%3.2fms)", size/t/1000/1000*16, t);		
	}
}

void AdlPrimitivesDemo::test( Buffer<int2>& buf, int size, Stopwatch& sw )
{
	Kernel* kernel = KernelManager::query( m_deviceData, "..\\..\\AdlDemos\\TestBed\\Demos\\AdlPrimitivesDemoKernel", "FillInt4Kernel" );
	Buffer<int4> constBuffer( m_deviceData, 1, BufferBase::BUFFER_CONST );


	int numGroups = (size+128*4-1)/(128*4);
	Buffer<u32> workBuffer0( m_deviceData, numGroups*(16) );
	Buffer<u32> workBuffer1( m_deviceData, numGroups*(16) );

	Buffer<int2> sortBuffer( m_deviceData, size );
	{
		int2* host = new int2[size];
		for(int i=0; i<size; i++)
		{
			host[i] = make_int2( getRandom(0, 0xf), i );
		}
		sortBuffer.write( host, size );
		DeviceUtils::waitForCompletion( m_deviceData );
		delete [] host;
	}

	int4 constData;
	{
		constData.x = size;
		constData.y = 0;
		constData.z = numGroups;
		constData.w = 0;
	}

	sw.start();

	int nThreads = size/4;
	{
		BufferInfo bInfo[] = { BufferInfo( &buf ), BufferInfo( &workBuffer0 ), BufferInfo( &workBuffer1 ) };
		Launcher launcher( m_deviceData, kernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( constBuffer, constData );
		launcher.launch1D( nThreads, 128 );
	}

	sw.split();

	{
		constData.w = 1;
		int nThreads = size/4;
		BufferInfo bInfo[] = { BufferInfo( &buf ), BufferInfo( &workBuffer0 ), BufferInfo( &workBuffer1 ) };
		Launcher launcher( m_deviceData, kernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( constBuffer, constData );
		launcher.launch1D( nThreads, 128 );
	}

	sw.split();

	{
		constData.w = 2;
		int nThreads = size/4;
		BufferInfo bInfo[] = { BufferInfo( &sortBuffer ), BufferInfo( &workBuffer0 ), BufferInfo( &workBuffer1 ) };
		Launcher launcher( m_deviceData, kernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( constBuffer, constData );
		launcher.launch1D( nThreads, 128 );
	}

	sw.stop();

	{
		int2* host = new int2[size];
		buf.read( host, size );
		DeviceUtils::waitForCompletion( m_deviceData );

		for(int i=0; i<128*4-1; i++)
		{
			ADLASSERT( host[i].x <= host[i+1].x );
		}

		delete [] host;
	}

	{
		float t[3];
		sw.getMs(t, 3);
		//	(byte * nElems)
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "LoadStore: %3.2fGB/s (%3.2fns)", (4*8*2)*nThreads/t[0]/1000/1000, t[0]*1000.f);		
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "GenHistog: %3.2fGB/s (%3.2fns)", (4*(8*2+2))*nThreads/t[1]/1000/1000, t[1]*1000.f);		
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "FullSort: %3.2fGB/s (%3.2fns)", (4*(8*2+2))*nThreads/t[2]/1000/1000, t[2]*1000.f);		
	}
}
