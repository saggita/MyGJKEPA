#include <GDemos/ClusteredDeferredDemo.h>
#include <AdlPrimitives/Sort/RadixSort.h>

#include <common/Physics/BoxShape.h>
#include <common/Physics/SimpleConvexShape.h>
#include <Common/Utils/ObjLoader.h>
#include <GDemos/MortonKeyUtils.h>


#if defined(DX11RENDER)

#endif

#if defined(DX11RENDER)
extern int g_wWidth;
extern int g_wHeight;

extern XMMATRIX g_ViewTr;
extern XMMATRIX g_ProjectionTr;


ClusteredDeferredDemo::ClusteredDeferredDemo(const Device* deviceData)
	: PostProcessBaseDemo( deviceData )
{
	{	//	build kernel
		const char *option = "-I ..\\";
		KernelBuilder<(DeviceType)MyDeviceType> builder;
		builder.setFromFile( m_deviceData, "GDemos\\ClusteredDeferredDemoKernel", option, true );
		builder.createKernel("PostProcessKernel", m_kernel );
	}

	m_buffer.allocate( m_deviceData, g_wWidth*g_wHeight );
	m_lightClusterBuffer.allocate( m_deviceData, MAX_LIGHTS/MAX_LIGHTS_PER_TILE );
	m_lightPosBuffer.allocate( m_deviceData, MAX_LIGHTS );
	m_lightColorBuffer.allocate( m_deviceData, MAX_LIGHTS );
}

ClusteredDeferredDemo::~ClusteredDeferredDemo()
{
	KernelBuilder<(DeviceType)MyDeviceType>::deleteKernel( m_kernel );
}

void ClusteredDeferredDemo::initDemo()
{
	ADLASSERT( MAX_LIGHTS/MAX_LIGHTS_PER_TILE <= 64 );

	{
		float4* cluster = new float4[MAX_LIGHTS/MAX_LIGHTS_PER_TILE];
		float4* p = new float4[MAX_LIGHTS];
		float4* c = new float4[MAX_LIGHTS];

		float x = 5.f;
		float y = 0.7f;
		float fallOffMul = 1.1f;

		Aabb space;
		space.setEmpty();
		for(int i=0; i<MAX_LIGHTS; i++)
		{
			p[i] = make_float4( getRandom(-x,x), getRandom(0.f, y), getRandom(-x,x), getRandom((fallOffMul-1.f), fallOffMul) );
			c[i] = make_float4( getRandom(0.f,1.f), getRandom(0.f,1.f), getRandom(0.f,1.f) );

//			p[i].y = y;
//			p[i].w = fallOffMul;

			space.includePoint( p[i] );
		}

		{
			DeviceUtils::Config cfg;
			Device* deviceHost = DeviceUtils::allocate( TYPE_HOST, cfg );
			{
				float4* pc = new float4[MAX_LIGHTS];
				memcpy( pc, p, MAX_LIGHTS*sizeof(float4) );
				HostBuffer<SortData> sortData( deviceHost, MAX_LIGHTS );
				MortonKeyUtils mUtins( space );

				for(int i=0; i<MAX_LIGHTS; i++)
				{
					sortData[i].m_key = mUtins.calcKeyXZ(p[i]);//(int)(p[i].x/x*0xffff);
					sortData[i].m_value = i;
				}

				{
					RadixSort<TYPE_HOST>::Data* dataS 
						= RadixSort<TYPE_HOST>::allocate( deviceHost, MAX_LIGHTS );

					RadixSort<TYPE_HOST>::execute( dataS, sortData, MAX_LIGHTS );

					RadixSort<TYPE_HOST>::deallocate( dataS );
				}

				for(int i=0; i<MAX_LIGHTS; i++)
				{
					p[i] = pc[sortData[i].m_value];
				}

				delete [] pc;
			}
			DeviceUtils::deallocate( deviceHost );
		}

		const float4 colors[] = { make_float4(0,1,1,1), make_float4(1,0,1,1), make_float4(1,1,0,1),
			make_float4(0,0,1,1), make_float4(0,1,0,1), make_float4(1,0,0,1) };

		for(int i=0; i<MAX_LIGHTS/MAX_LIGHTS_PER_TILE; i++)
		{
			Aabb& aabb = m_clusterAabbs[i];
			aabb.setEmpty();
			for(int j=0; j<MAX_LIGHTS_PER_TILE; j++)
			{
				int idx = i*MAX_LIGHTS_PER_TILE + j;

				Aabb jAabb;
				jAabb.m_max = p[idx] + make_float4( p[idx].w );
				jAabb.m_min = p[idx] - make_float4( p[idx].w );

				aabb.includeVolume( jAabb );
				c[idx] = colors[i%6];

				if( i==0 ) c[idx] = make_float4(1.f);
			}

			float4 center = aabb.center();
			float4 hExt = aabb.m_max - center;
			center.w = length3( hExt );

			cluster[i] = center;
		}

//		p[0] = make_float4(0,fallOffMul*0.5f,x,fallOffMul);
//		c[0] = make_float4(1,0,0,1);

		m_lightClusterBuffer.write( cluster, MAX_LIGHTS/MAX_LIGHTS_PER_TILE );
		m_lightPosBuffer.write( p, MAX_LIGHTS );
		m_lightColorBuffer.write( c, MAX_LIGHTS );
		DeviceUtils::waitForCompletion( m_deviceData );

		delete [] cluster;
		delete [] p;
		delete [] c;
	}
}


void ClusteredDeferredDemo::renderPre()
{
	prepareGBuffer();
}


void ClusteredDeferredDemo::renderPost()
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

	Stopwatch dsw( m_deviceData );
	dsw.start();
	{	//	run CS
		BufferDX11<int> cBuffer;
		cBuffer.m_srv = m_colorRT.m_srv;

		BufferDX11<int> pBuffer;
		pBuffer.m_srv = m_posRT.m_srv;

		BufferDX11<int> nBuffer;
		nBuffer.m_srv = m_normalRT.m_srv;

		BufferInfo bInfo[] = { BufferInfo( &m_lightPosBuffer, true ), BufferInfo( &m_lightColorBuffer, true ),
			BufferInfo( &cBuffer, true ), BufferInfo( &pBuffer, true ),
			BufferInfo( &nBuffer, true ), BufferInfo( &m_lightClusterBuffer, true ),
			BufferInfo( &m_buffer ) };

		Launcher launcher( m_deviceData, &m_kernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
//		launcher.pushBackR( m_lightPosBuffer );
//		launcher.pushBackR( m_lightColorBuffer );
//		launcher.pushBackR( cBuffer );
//		launcher.pushBackR( pBuffer );
//		launcher.pushBackR( nBuffer );
//		launcher.pushBackR( m_lightClusterBuffer );
//		launcher.pushBackRW( m_buffer );
		launcher.setConst( g_constBuffer, cb );
		launcher.launch2D( nClusterX*TILE_SIZE, nClusterY*TILE_SIZE, TILE_SIZE, TILE_SIZE );
	}
	dsw.stop();

	{
		m_nTxtLines = 0;
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "%3.3fms", dsw.getMs());
	}


	resolve( &m_buffer.m_srv );
}

int ClusteredDeferredDemo::calcNumTiles(int size, int clusterSize)
{
	return max2( 1, (size/clusterSize)+(!(size%clusterSize)?0:1) );
}
#endif
