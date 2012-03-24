#include <GDemos/PathTraceDemo.h>
#include <omp.h>

extern int g_wWidth;
extern int g_wHeight;

extern DeviceShaderDX11 g_bufferToRTPixelShader;



void PathTraceDemo::Sphere::intersect(const Ray& ray, IntersectOut& out)
{
	out.m_dist = -1.f;

	float4 op = ray.m_orig - m_pos;
	float4 d = ray.m_dir;
	float r = m_pos.w;

	float b = dot3F4( op, d );
	float t = b*b - dot3F4( op, op ) + r*r;

	if( t < 0.f ) return;
	t = sqrtf( t );

	float dist = -b-t;
	if( dist < 0.f )
	{
		dist = -b+t;
	}
	if( dist < 0.f ) return;

	out.m_dist = dist;
	out.m_normal = normalize3( ray.m_orig+dist*ray.m_dir - m_pos );
}

PathTraceDemo::PathTraceDemo(const Device* device)
	: Demo(device)
{
	m_frameBuffer = new float4[ g_wWidth*g_wHeight ];
	m_buffer = new Buffer<float4>( m_deviceData, g_wWidth*g_wHeight );
	m_sphere = new Sphere[NUM_SPHERES];
}

PathTraceDemo::~PathTraceDemo()
{
	delete [] m_frameBuffer;
	delete m_buffer;
	delete [] m_sphere;
}


float4 getRandomSphericalPoint()
{
	float theta = getRandom(0.f, 2*PI);
	float u = getRandom(-1.f, 1.f);

	float v = sqrtf(1.f-u*u);

	float4 p = make_float4( v*cos(theta), v*sin(theta), u );
	return normalize3( p );
}

int PathTraceDemo::intersect(const Ray& ray, IntersectOut& minHit)
{
	int minIdx = -1;
	minHit.m_dist = FLT_MAX;
	for(int ie=0; ie<NUM_SPHERES; ie++)
	{
		IntersectOut hit;
		m_sphere[ie].intersect( ray, hit );

		if( hit.hasHit() )
		{
			if( hit.m_dist < minHit.m_dist ) 
			{
				minHit = hit;
				minIdx = ie;
			}
		}
	}
	return minIdx;
}

void PathTraceDemo::init()
{
	m_time = 0.f;

	m_camera.m_from = make_float4(0, 1, -1.5f);
	m_camera.m_to = make_float4(0,0,0);
	m_camera.m_up = make_float4(0,1,0);
	m_camera.m_fov = 70.f;

	{
		float h = 0.30f;
		float r = 1e+3;
		m_sphere[0].m_pos = make_float4(0,-h-r,0,r);
//		m_sphere[2].m_pos = make_float4(-2*h-r,0,0,r);
//		m_sphere[3].m_pos = make_float4(2*h+r,0,0,r);
//		m_sphere[4].m_pos = make_float4(0,0,-20*h-r,r);
//		m_sphere[5].m_pos = make_float4(0,0,20*h+r,r);

		r = 0.2f;
		m_sphere[1].m_pos = make_float4(0,r*7.f,0,2.f*r);
		m_sphere[1].m_color = make_float4(1,1,1,0);
		m_sphere[1].m_emission = make_float4(5.f);//1,1,1,1);


		float t = 2*PI/3.f;
		float rr = r*1.1f;
		m_sphere[2].m_pos = make_float4(rr*sin(t),-h+r,rr*cos(t),r);
		m_sphere[2].m_color = make_float4(1,1,1,0);
		m_sphere[2].m_emission = make_float4(5.f);

		m_sphere[3].m_pos = make_float4(rr*sin(2*t),-h+r,rr*cos(2*t),r);
		m_sphere[3].m_color = make_float4(0,1,0,0);

		m_sphere[4].m_pos = make_float4(rr*sin(3*t),-h+r,rr*cos(3*t),r);
		m_sphere[4].m_color = make_float4(0,0,1,0);

	}

	for(int j=0; j<g_wHeight; j++) for(int i=0; i<g_wWidth; i++)
	{
		m_frameBuffer[i+(g_wHeight-j-1)*g_wWidth] = make_float4( 0.f );
	}

	int i = 0;

	{
		step(0.f);
	}

}

void PathTraceDemo::step(float dt)
{
	adlDebugPrintf("%d start\n",m_stepCount);

	int windowSize = max2( g_wWidth, g_wHeight );

	float xExtent = tan(m_camera.m_fov/180.f/2.f*PI)*length3( m_camera.m_from-m_camera.m_to );

	float4 dy = normalize3( m_camera.m_up );///g_wHeight*xExtent;
	float4 dx = normalize3( cross3( m_camera.m_to - m_camera.m_from, dy ) )/(float)g_wWidth*xExtent;
	dy = normalize3( cross3( dx, m_camera.m_to - m_camera.m_from ) )/(float)g_wHeight*xExtent;
	float4 orig = m_camera.m_to - (float)g_wHeight*dy/2.f - (float)g_wWidth*dx/2.f;

	Stopwatch sw;

	sw.start();
//	for(int ii=0; ii<(PATH_TRACE)?10:1; ii++)
	int nii = (PATH_TRACE)?5:1;
	for(int ii=0; ii<nii; ii++)
	{
#pragma omp parallel for
		for(int j=0; j<g_wHeight; j++) for(int i=0; i<g_wWidth; i++)
		{
			float4 color = make_float4(0.f);

			float4 v = orig + (float)i*dx + (float)j*dy - m_camera.m_from;
			v = normalize3(v);

			Ray ray( m_camera.m_from, v );

			if(!PATH_TRACE)
			{
				IntersectOut minHit;
				int hitIdx = intersect( ray, minHit );
				if( hitIdx != -1 )
				{
					float4 lPos = make_float4(0.f,1.f,0.f,0.f);
					float4 hitPoint = ray.m_orig + minHit.m_dist*ray.m_dir;
					float4 lightVec = normalize3( lPos - hitPoint );

					color = dot3F4(minHit.m_normal, lightVec )*m_sphere[hitIdx].m_color;
					color.w = 1.f;
				}
				m_frameBuffer[i+(g_wHeight-j-1)*g_wWidth] = color;
			}
			else
			{
				float4 fraction = make_float4(1.f);
				int nIter = 10;
				for(int iter=0; iter<nIter; iter++)
				{
					IntersectOut hit;
					int hitIdx = intersect( ray, hit );
					if( hitIdx == -1 ) { break; };

					float4 hitColor = m_sphere[hitIdx].m_color;
					float4 hitEmission = m_sphere[hitIdx].m_emission;
					float4 hitPoint = ray.m_orig + hit.m_dist*ray.m_dir;
					float4 hitNormal = hit.m_normal;
					float4 randomVector = getRandomSphericalPoint();
					if( dot3F4( hitNormal, randomVector ) < 0.f ) randomVector *= -1.f;

					ADLASSERT( dot3F4( hitNormal, randomVector ) >= 0.f );

					color += fraction*hitEmission;
					fraction *= hitColor;

					ray.m_orig = hitPoint + (1e-3f)*hitNormal;
					ray.m_dir = randomVector;
				}

				m_frameBuffer[i+(g_wHeight-j-1)*g_wWidth] += make_float4(color.x, color.y, color.z, 1.f);
			}
		}
	}
	sw.stop();

	m_time = sw.getMs();
}

void PathTraceDemo::render()
{

}

void PathTraceDemo::renderPost()
{
	{
		float4* tmp = new float4[g_wWidth*g_wHeight];
		for(int j=0; j<g_wHeight; j++) for(int i=0; i<g_wWidth; i++)
		{
			float n = m_frameBuffer[i+(g_wHeight-j-1)*g_wWidth].w;
			tmp[i+(g_wHeight-j-1)*g_wWidth] = m_frameBuffer[i+(g_wHeight-j-1)*g_wWidth]/n;

		}

		m_buffer->write( tmp, g_wWidth*g_wHeight );
		delete [] tmp;
	}

	{
		DeviceDX11* dd = (DeviceDX11*)m_deviceData;

		dd->m_context->PSSetShaderResources( 0, 1, ((BufferDX11<float4>*)m_buffer)->getSRVPtr() );

		//	render to screen
		renderFullQuad( m_deviceData, &g_bufferToRTPixelShader, 
			make_float4((float)g_wWidth, (float)g_wHeight, 0,0 ) );

		ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
		dd->m_context->PSSetShaderResources( 0, 1, ppSRVNULL );
	}

	{
		m_nTxtLines = 0;
		sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "%d [%3.2fms]", m_stepCount, m_time);
	}
}
