#ifndef TEST_DEMO_H
#define TEST_DEMO_H

#include <Adl/Adl.h>
#include <AdlPrimitives/Math/Math.h>
#include <AdlPrimitives/Math/Array.h>
#include <Common/Base/ThreadPool.h>
#include <stdio.h>
#include <common/Physics/BoxShape.h>

#include <AdlGraphics/DeviceDraw.h>


extern DeviceShaderDX11 g_quadVertexShader;
extern DeviceShaderDX11 g_quadPixelShader;
extern adl::DeviceDX11* g_deviceData;


namespace adl
{

#define COMPUTE_DX11 enum { MyDeviceType = TYPE_DX11,};\
		typedef Launcher::BufferInfo BufferInfo;

#define COMPUTE_CL enum { MyDeviceType = TYPE_CL,};\
		typedef Launcher::BufferInfo BufferInfo;

#define INITIALIZE_DEVICE_DATA( deviceData ) 	\
	if( deviceData ) \
	{ \
		if( deviceData->m_type == MyDeviceType ) \
		{ \
			m_device = (Device*)deviceData; \
			m_ddCreated = false; \
		} \
	} \
	if( !m_device )\
	{ \
		DeviceUtils::Config cfg; \
		cfg.m_deviceIdx = 3;\
		m_device = DeviceUtils::allocate( (DeviceType)MyDeviceType, cfg ); \
		m_ddCreated = true; \
	}

#define INITIALIZE_DEVICE_DATA1( deviceData, deviceIdx ) 	\
	if( deviceData ) \
	{ \
		if( deviceData->m_type == MyDeviceType ) \
		{ \
			m_device = (Device*)deviceData; \
			m_ddCreated = false; \
		} \
	} \
	if( !m_device )\
	{ \
		DeviceUtils::Config cfg; \
		cfg.m_deviceIdx = deviceIdx;\
		m_device = DeviceUtils::allocate( (DeviceType)MyDeviceType, cfg ); \
		m_ddCreated = true; \
	}


#define DESTROY_DEVICE_DATA if( m_ddCreated ) { DeviceUtils::deallocate( m_device ); m_ddCreated=false; }


#define ADD_EXTENSION(deviceData, fullPath, fileName) \
	if( deviceData->m_type == DeviceData::TYPE_DX11) sprintf_s(fullPath, 256, "%s.hlsl", fileName);\
	else sprintf_s(fullPath, 256, "%s.cl", fileName);


__inline
float4 rainbowMap( float s )
{
	float c = 4.f;
	float r,g,b;
	r = c*(s-0.75f);
	g = c*(s-0.5f);
	b = c*(s-0.25f);

	float4 col = make_float4( 1.f-r*r, 1.f-g*g, 1.f-b*b );
	return col;
}


class Demo
{
	public:
		__inline
		Demo(int nThreads = 1);

		__inline
		Demo( const Device* deviceData, void* option = NULL, int nThreads = 1 );

		__inline
		virtual ~Demo();

		void stepDemo(){ m_stepCount++; step(m_dt); }

		void resetDemo() { m_stepCount=0; reset(); }

		virtual void init(){}

		virtual void step(float dt){ m_stepCount++; }

		virtual void render(){}

		virtual void renderPre(){}

		virtual void renderPost(){}

		virtual void reset(){}

		virtual void keyListener(unsigned char key){}

		virtual void keySpecialListener(unsigned char key){}

		void setRenderFilter(u32 filterBits = 0xffffffff) { m_renderFilter = filterBits; }

		__inline
		void timerReset()
		{
			if( m_threads )
			{
				m_threads->resetThreadTimer();
			}
		}

		__inline
		void timerStart(u16 type = 0)
		{
			if( m_threads )
			{
				ThreadPool::Thread& th = m_threads->m_threads[0];

				LARGE_INTEGER s;
				QueryPerformanceCounter( &s );

				float t;
				t = (float)(1000*(s.QuadPart - m_threads->m_startTime.QuadPart))/m_threads->m_frequency.QuadPart;
				th.m_timestamps[ th.m_nTimestamps ].m_start = t;
				th.m_timestamps[ th.m_nTimestamps ].m_type = type;
//				m_threads->m_threads[0].m_nTimestamps++;
			}
		}

		__inline
		void timerEnd()
		{
			if( m_threads )
			{
				ThreadPool::Thread& th = m_threads->m_threads[0];

				LARGE_INTEGER s;
				QueryPerformanceCounter( &s );

				float t;
				t = (float)(1000*(s.QuadPart - m_threads->m_startTime.QuadPart))/m_threads->m_frequency.QuadPart;
				th.m_timestamps[ th.m_nTimestamps ].m_end = t;
				th.m_nTimestamps++;
			}
		}
		__inline
		void drawThreadProfile();

		__inline
		void drawBar1(float start, float end, const float4& color, int column = 0);

		__inline
		static
		void drawShape( const ShapeBase* box, const float4& pos, const Quaternion& quat, const float4& color );

	public:
		Device* m_device;
		bool m_ddCreated;

		ThreadPool* m_threads;

		float m_dt;
		float m_profilerDt;
		int m_stepCount;
		bool m_enableLighting;
		bool m_enablePostEffect;
		bool m_enableAlphaBlending;
		bool m_enableBackFaceCulling;
		float4 m_backgroundColor;
		u32 m_renderFilter;
		float4 m_cameraPos;


		enum
		{
			MAX_LINES = 40,
			LINE_CAPACITY = 512,
		};

		int m_nTxtLines;
		char m_txtBuffer[MAX_LINES][LINE_CAPACITY];
};

Demo::Demo(int nThreads): 
m_dt(1.f/60.f), m_stepCount(0), m_enableLighting(true), m_enablePostEffect(false), m_enableAlphaBlending(false), m_enableBackFaceCulling(true), m_nTxtLines(0)
{
	m_profilerDt = m_dt;
	m_ddCreated = false;
	m_device = 0;

	m_threads = 0;
	if( nThreads )
	{
		m_threads = new ThreadPool( nThreads );
	}
	m_backgroundColor = make_float4(0.f);
	setRenderFilter();
	m_cameraPos = make_float4(0,0,-4);
}


Demo::Demo( const Device* deviceData, void* option, int nThreads )
: m_dt(1.f/60.f), m_stepCount(0), m_enableLighting(true), m_enablePostEffect(false), m_enableAlphaBlending(false), m_enableBackFaceCulling(true)
{
	m_profilerDt = m_dt;
	m_ddCreated = true;

	if( deviceData )
	{
		m_device = (Device*)deviceData;
		m_ddCreated = false;
	}

	if( m_ddCreated )
	{
		DeviceUtils::Config cfg;
#ifdef COMPUTE_DEVICE_DX11
		m_device = DeviceUtils::allocate( TYPE_DX11, cfg );
#endif

#ifdef COMPUTE_DEVICE_CL
		m_device = DeviceUtils::allocate( TYPE_CL, cfg );
#endif
	}


	m_threads = 0;
	if( nThreads )
	{
		m_threads = new ThreadPool( nThreads );
	}
	m_nTxtLines = 0;

	m_backgroundColor = make_float4(0.f);
	setRenderFilter();
	m_cameraPos = make_float4(0,0,-4);
}

Demo::~Demo()
{
	if( m_ddCreated )
	{
		DeviceUtils::deallocate( m_device );
	}

	if( m_threads )
	{
		delete m_threads;
	}
}


#ifdef DX11RENDER

void Demo::drawBar1(float start, float end, const float4& color, int column)
{
//	start += 0.001f;

	start = 2*start-1.f;
	end = 2*end-1.f;

	float s = 0.025f;
	float4 v[] = {make_float4(1.f-s*(column+0.9f),start,0), make_float4(1.f-s*column,start,0), 
		make_float4(1.f-s*column,end,0), make_float4(1.f-s*(column+0.9f),end,0)};

	XMFLOAT3& a = *(XMFLOAT3*)&v[0];
	XMFLOAT3& b = *(XMFLOAT3*)&v[2];
	XMFLOAT3& c = *(XMFLOAT3*)&v[1];
	XMFLOAT3& d = *(XMFLOAT3*)&v[3];
	XMFLOAT4 c4 = XMFLOAT4(color.x,color.y,color.z,0.25f);

	VertexColorStruct vtx[4];
	{
		vtx[0].m_pos = a;
		vtx[0].m_normal = XMFLOAT3(0,0,0);
		vtx[0].m_color = c4;
		vtx[1].m_pos = b;
		vtx[1].m_normal = XMFLOAT3(0,0,0);
		vtx[1].m_color = c4;
		vtx[1].m_color.w = 1.f;
		vtx[2].m_pos = c;
		vtx[2].m_normal = XMFLOAT3(0,0,0);
		vtx[2].m_color = c4;
		vtx[3].m_pos = d;
		vtx[3].m_normal = XMFLOAT3(0,0,0);
		vtx[3].m_color = c4;
		vtx[3].m_color.w = 1.f;
	}
	u32 idx[] = {0,1,2, 0,3,1};

	RenderObject& obj = g_debugRenderObj.expandOne();
	new (&obj)RenderObject();
	obj.m_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	obj.m_vtxBuffer = g_appDebugVertexBuffer->getBuffer();
	obj.m_idxBuffer = g_appDebugIndexBuffer->getBuffer();
	obj.m_nIndices = 6;
	obj.m_vtxOffset = g_appDebugVertexBuffer->append( 4, vtx );
	obj.m_idxOffset = g_appDebugIndexBuffer->append( 6, idx );
	obj.m_vertexShader = g_quadVertexShader;
	obj.m_pixelShader = g_quadPixelShader;
}
#else
void Demo::drawBar1(float start, float end, const float4& color, int column)
{

}
#endif


void Demo::drawThreadProfile()
{
	if( !m_threads ) return;

	float total = m_profilerDt*1000.f;
	int nThreads = m_threads->m_nThreads;

	for(int it=0; it<nThreads; it++)
	{
		drawBar1(0,1,make_float4(0.1f),it);
	}

	int nCtables = 3;
	float4 ctable[] = {make_float4(0,0,1,1), make_float4(0,1,0,1), make_float4(1,0,0,1)};

	for(int it=0; it<nThreads; it++)
	{
		for(int i=0; i<m_threads->m_threads[it].m_nTimestamps; i++)
		{
			float s = m_threads->m_threads[it].m_timestamps[i].m_start/total;
			float e = m_threads->m_threads[it].m_timestamps[i].m_end/total;

			float4 c = ctable[m_threads->m_threads[it].m_timestamps[i].m_type%nCtables];
			c += 0.1f;
			drawBar1( s,e,c, it );
		}
	}



	for(int i=1; i<4; i++)
	{
		float s = 0.25f;
		float4 c = make_float4(1.0f, 0.5f, 0.0f, 1.0f);
		for(int it=0; it<nThreads; it++)
			drawBar1(s*i, s*(i+0.005f),c,it);
	}
}

void Demo::drawShape( const ShapeBase* box, const float4& pos, const Quaternion& quat, const float4& color )
{
	ShapeBase* boxShape = (ShapeBase*)box;

	const float4* vtx = boxShape->getVertexBuffer();
	const int4* tris = boxShape->getTriangleBuffer();

	float4* v = new float4[boxShape->getNumTris()*3];
	u32* idx = new u32[boxShape->getNumTris()*3];
	float4* n = new float4[boxShape->getNumTris()*3];

/*
	float c = 0.4f;
	float4 color = make_float4(0.8f-c,0.8f-c,0.8f,1.f)*1.8f;
	color = make_float4(1.0f,0.5f,0.5f,1);

	if( i==0 ) color = make_float4(1.f,1.f,1.f, 1.f);
*/
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

};

#endif
