#include <GDemos/SssDemo.h>
#include <common/Physics/BoxShape.h>


SssDemo::SssDemo( const Device* device )
	: Demo( device ) 
{
	{
		ShaderUtilsDX11 builder( m_deviceData, "GDemos\\SssShader.hlsl" );
		builder.createPixelShader( "SphereSssPixelShader", m_sphereSssPShader );
		builder.createPixelShader( "BoxSssPixelShader", m_boxSssPShader );
	}

	m_lightBuffer.allocate( m_deviceData, MAX_LIGHTS );


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
}

SssDemo::~SssDemo()
{
	for(int i=0; i<MAX_BODIES; i++)
	{
		delete m_shapes[i];
	}

	ShaderUtilsDX11::deleteShader( m_sphereSssPShader );
	ShaderUtilsDX11::deleteShader( m_boxSssPShader );
}

void SssDemo::reset()
{
	float4 host[MAX_LIGHTS];
	host[0] = make_float4(0,4,2,1);
	for(int i=1; i<MAX_LIGHTS; i++)
	{
		host[i] = make_float4(0,0,4,1);
	}
	m_lightBuffer.write( host, MAX_LIGHTS );
	DeviceUtils::waitForCompletion( m_deviceData );
}

void SssDemo::step(float dt)
{
	float theta = m_stepCount*dt*0.25f;

	float r = 1.f;
	float4 host = make_float4( sin(theta), 0.5f, cos(theta) )*r;
	host.w = 1.f;

	m_lightBuffer.write( &host, 1, 1 );
	DeviceUtils::waitForCompletion( m_deviceData );
}

void SssDemo::render()
{

	if( TEST_SPHERE )
	{
		if(0)
		{
			float4 vtx[1] = { make_float4(0.f) };
			float4 color[1] = { make_float4(0.f, 1.f, 0.f) };
			float2 radius[1] = { make_float2(0.5f) };

			pxDrawPointSprite( vtx, color, radius, 1 );
		}
		else
		{
			const int N = 5;
			float4 vtx[N*N];
			float4 color[N*N];
			float2 radius[N*N];

			float4 colors[] = { make_float4(1,0,0), make_float4(0,1,0), make_float4(0,0,1) };

			float r = 0.15f;
			float s = r*4.f;
			for(int i=0; i<N; i++) for(int j=0; j<N; j++)
			{
				int idx = i*N+j;
				vtx[idx] = make_float4( s*(i-(N>>1)), -0.f, s*(j-(N>>1)) );
				color[idx] = colors[idx%3];
				radius[idx] = make_float2( r );
			}

			pxDrawPointSprite( vtx, color, radius, N*N );
		}

		{
			RenderObject& obj = g_debugRenderObj[ g_debugRenderObj.getSize()-1 ];
			obj.m_pixelShader = m_sphereSssPShader;
			int idx = obj.m_nResources;
			obj.m_resources[idx].m_buffer = (BufferDX11<float4>&)m_lightBuffer;
			obj.m_resources[idx].m_type = RenderObject::Resource::PIXEL_SHADER;
			obj.m_nResources++;
		}

		pxDrawPoint( make_float4(0,0,2.f), make_float4(0, 1, 0) );
	}
	else
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


			float4 color = make_float4(1.f,1.f,1.f, 1.f);

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

			if( i!= 0 )
			{
				RenderObject& obj = g_debugRenderObj[ g_debugRenderObj.getSize()-1 ];
				obj.m_pixelShader = m_boxSssPShader;
				int idx = obj.m_nResources;
				obj.m_resources[idx].m_buffer = (BufferDX11<float4>&)m_lightBuffer;
				obj.m_resources[idx].m_type = RenderObject::Resource::PIXEL_SHADER;
				obj.m_nResources++;
			}

			delete [] v;
			delete [] idx;
			delete [] n;
		}
	}
}

