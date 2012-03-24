#include <Demos/MixedParticlesDemo.h>
#include <Demos/Bp/SweepBroadphase.h>
#include <Common/Geometry/Aabb.h>

extern int g_wWidth;
extern int g_wHeight;

extern DeviceShaderDX11 g_defaultPixelShader;
extern ID3D11SamplerState* g_defaultSampler;
extern BufferDX11<ConstantBuffer> g_constBuffer;
extern DeviceShaderDX11 g_bufferToRTPixelShader;
extern DeviceShaderDX11 g_textureMapWColorPixelShader;
extern DeviceRenderTargetDX11 g_depthStencil;

extern XMMATRIX g_ViewTr;
extern XMMATRIX g_ProjectionTr;


#define RESOLUTION_FACTOR 1.5f

__inline
void calcBoundary(float4* p0, float4* v0, float4* fOut0, int n0, float dt, float e,
							float4* planes, int NUM_PLANES)
{
	for(int i=0; i<n0; i++)
	{
		float4 f = make_float4(0,0,0,0);
		float sCoeff, dCoeff;
		{
			float m = v0[i].w/2.f;
			sCoeff = m/(dt*dt);
			dCoeff = m/dt*(1.f-e);
		}
		for(int j=0; j<NUM_PLANES; j++)
		{
			const float4& eqn = planes[j];
			float dist = dot3w1( p0[i], eqn );
			float r_i = p0[i].w;
			if( dist < r_i )
			{
				f += sCoeff*(r_i-dist)*eqn;
				f += dCoeff*(-v0[i]);
			}
		}
		fOut0[i] += f;
	}
}

__inline
void integration(float4* p0, float4* v0, float4* f0, int n0, float dt, const float4& g)
{
	for(int i=0; i<n0; i++)
	{

		if( v0[i].w == FLT_MAX ) continue;


		float4 x = p0[i];
		float4 v = v0[i];

		v += f0[i]*dt/v.w+g;
		x += v*dt;

		p0[i] = make_float4(x.x, x.y, x.z, p0[i].w);
		v0[i] = make_float4(v.x, v.y, v.z, v0[i].w);
		f0[i] = make_float4(0,0,0,0);
	}
}

__inline
float4 calcForce(const float4& x_i, const float4& x_j, const float4& v_i, const float4& v_j, float r_i, float r_j, float m_i, float m_j, float dt,
				 float e = 0.7f)
{
	float4 f = make_float4(0,0,0,0);
	float sCoeff, dCoeff;

	{
		float dtInv = 1.f/dt;
		float m = (m_i*m_j)/(m_i+m_j);
		sCoeff = m*dtInv*dtInv;
		dCoeff = m*dtInv*(1.f-e);
	}

	float4 x_ij = x_j-x_i;
	float dist2 = dot3F4( x_ij, x_ij );

	if( dist2 < pow( r_i+r_j, 2.f ) )
	{
		float dist = sqrtf( dist2 );
		f -= sCoeff*(r_i+r_j-dist)*x_ij/dist;
		f += dCoeff*(v_j - v_i);
	}
	return f;
}

__inline
int4 ugConvertToGridCrd(const float4& pos, float gridScale)
{
	int4 g;
	g.x = (int)floor(pos.x*gridScale);
	g.y = (int)floor(pos.y*gridScale);
	g.z = (int)floor(pos.z*gridScale);
	return g;
}

__inline
int ugGridCrdToGridIdx(const int4& g, int nCellX, int nCellY, int nCellZ)
{
	return g.x+g.y*nCellX+g.z*nCellX*nCellY;
}

__inline
void calcInteractions(float4* p0, float4* v0, float4* fOut0, int n0,
					  float4* p1, float4* v1, float4* fOut1, int n1,
					  float dt, float e, UniformGrid<TYPE_CL>::Data* ugData,
					  MapBuffer* gridCounterM, MapBuffer* gridDataM)
{
	if( ugData )
	{
		const int4& nCells = ugData->m_nCells;// grid->m_gProps.m_nCells;
		int* gridCounter = gridCounterM->getPtr<int>();
		int* gridData = gridDataM->getPtr<int>();
		float rS = p1[0].w;

		for(int iIdx=0; iIdx<n0; iIdx++)
		{
			const float4& x_i = p0[iIdx];
			const float4& v_i = v0[iIdx];
			float r_i = x_i.w;

			int4 iGridCrdMin = ugConvertToGridCrd( x_i-ugData->m_min-r_i-rS, ugData->m_gridScale );
			int4 iGridCrdMax = ugConvertToGridCrd( x_i-ugData->m_min+r_i+rS, ugData->m_gridScale );

			for(int i=iGridCrdMin.x; i<=iGridCrdMax.x; i++)
			for(int j=iGridCrdMin.y; j<=iGridCrdMax.y; j++)
			for(int k=iGridCrdMin.z; k<=iGridCrdMax.z; k++)
			{
				int4 gridCrd = make_int4( i,j,k,0 );

				if( gridCrd.x < 0 || gridCrd.x >= nCells.x
					|| gridCrd.y < 0 || gridCrd.y >= nCells.y
					|| gridCrd.z < 0 || gridCrd.z >= nCells.z ) continue;
				
				int gridIdx = ugGridCrdToGridIdx( gridCrd, nCells.x, nCells.y, nCells.z );
				int numElem = gridCounter[gridIdx];

				numElem = min2(MAX_IDX_PER_GRID, numElem);

				for(int ie=0; ie<numElem; ie++)
				{
					int jIdx = gridData[ MAX_IDX_PER_GRID*gridIdx+ie ];

					const float4& x_j = p1[jIdx];
					const float4& v_j = v1[jIdx];

					float4 f = calcForce( x_i, x_j, v_i, v_j, x_i.w, x_j.w, v_i.w, v_j.w, dt, e );

					fOut0[iIdx] += f;
					fOut1[jIdx] -= f;
				}
			}
		}
	}
	else
	{
		for(int i=0; i<n0; i++)
		{
			for(int j=0; j<n1; j++)
			{
				if( p0[i].x == p1[j].x && p0[i].y == p1[j].y && p0[i].z == p1[j].z ) continue;

				float4 f = calcForce( p0[i], p1[j], v0[i], v1[j], p0[i].w, p1[j].w, v0[i].w, v1[j].w, dt, e );

				fOut0[i] += f;
				fOut1[j] -= f;
			}
		}
	}
}

//---

MixedParticlesDemo::MixedParticlesDemo( const Device* device )
	: Demo(4), m_ugData(0), m_hostUgData(0)
{
	m_enableAlphaBlending = true;

	INITIALIZE_DEVICE_DATA( device );

	m_backgroundColor = make_float4( 1.f, 1.f, 1.f, 0.f );

	m_nL = 200;
	m_nS = 8*1024;

	m_pos = new float4[ m_nL ];
	m_vel = new float4[ m_nL ];
	m_force = new float4[ m_nL ];
	m_aabb = new Aabb[ m_nL ];
	m_aabbUint = new AabbUint[ m_nL ];

	BufferBase::BufferType type = BufferBase::BUFFER_ZERO_COPY;

	m_posSD.allocate( m_deviceData, m_nS, type );
	m_velSD.allocate( m_deviceData, m_nS, type );
	m_forceSD.allocate( m_deviceData, m_nS );
	m_forceIntD.allocate( m_deviceData, m_nS, type );
	m_constBuffer.allocate( m_deviceData, 1 );
	m_posSNative.allocate( m_deviceData, m_nS );
	m_velSNative.allocate( m_deviceData, m_nS );

	m_posMapped.map( m_deviceData, m_posSD, m_nS );
	m_velMapped.map( m_deviceData, m_velSD, m_nS );
	m_forceMapped.map( m_deviceData, m_forceIntD, m_nS );

	DeviceUtils::waitForCompletion( m_deviceData );

	{
		const char *option = "-I ..\\";
		KernelBuilder<TYPE_CL> builder;
		builder.setFromFile( m_deviceData, "Demos\\MixedParticlesKernels", option, true );
		builder.createKernel( "CollideAllKernel", m_collideAllKernel );
		builder.createKernel( "CollideGridKernel", m_collideGridKernel );
		builder.createKernel( "IntegrateKernel", m_integrateKernel );
		builder.createKernel( "CopyPosVelKernel", m_copyPosVelKernel );
	}


	m_planes[0] = make_float4(0,1,0,1);
	m_planes[1] = make_float4(-1,0,0,1);
	m_planes[2] = make_float4(1,0,0,1);
	m_planes[3] = make_float4(0,-1,0,1);

	{
		DeviceUtils::Config hostCfg;
		m_deviceHost = DeviceUtils::allocate( TYPE_HOST, hostCfg );
	}

	if( ENABLE_POST_EFFECT )
	{	//	init render
		ADLASSERT( device->m_type == TYPE_DX11 );
		m_deviceRender = (DeviceDX11*)device;

		DeviceRenderTargetDX11::createRenderTarget( m_deviceRender, g_wWidth, g_wHeight, m_colorRT );

		{
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			};

			ShaderUtilsDX11 builder( m_deviceRender, "GDemos\\Deferred.hlsl" );
			builder.createVertexShader( "DeferredGPVS", m_gpVShader, ARRAYSIZE( layout ), layout );
			builder.createPixelShader( "DeferredGPPS", m_gpPShader );
		}

		m_gVShader = g_defaultVertexShader;
		m_gPShader = g_defaultPixelShader;

		{
			KernelBuilder<TYPE_DX11> builder;
			builder.setFromFile( m_deviceRender, "Demos\\MixedParticlesShader.hlsl" );
			builder.createKernel( "PostProcessKernel", m_postProcessKernel );
		}

		m_renderBuffer.allocate( m_deviceRender, g_wWidth*g_wHeight );
		m_rConstBuffer.allocate( m_deviceRender, 1, BufferBase::BUFFER_CONST );
	}
}

MixedParticlesDemo::~MixedParticlesDemo()
{
	delete [] m_pos;
	delete [] m_vel;
	delete [] m_force;
	delete [] m_aabb;
	delete [] m_aabbUint;

	m_posMapped.unmap( m_posSD, m_nS );
	m_velMapped.unmap( m_velSD, m_nS );
	m_forceMapped.unmap( m_forceIntD, m_nS );

	KernelBuilder<TYPE_CL>::deleteKernel( m_collideAllKernel );
	KernelBuilder<TYPE_CL>::deleteKernel( m_collideGridKernel );
	KernelBuilder<TYPE_CL>::deleteKernel( m_integrateKernel );
	KernelBuilder<TYPE_CL>::deleteKernel( m_copyPosVelKernel );

	int nCells = m_ugData->m_nCells.x*m_ugData->m_nCells.y*m_ugData->m_nCells.z;
	if( m_ugData )
	{
		m_gridMapped.unmap( *m_ugData->m_gridData, nCells );
		m_gridCounterMapped.unmap( *m_ugData->m_counter, nCells );
		delete m_ugData;
	}


	DeviceUtils::deallocate( m_deviceHost );


	if( ENABLE_POST_EFFECT )
	{	//	delete render
		DeviceRenderTargetDX11::deleteRenderTarget( m_deviceData, m_colorRT );
		ShaderUtilsDX11::deleteShader( m_gpVShader );
		ShaderUtilsDX11::deleteShader( m_gpPShader );

		KernelBuilder<TYPE_DX11>::deleteKernel( m_postProcessKernel );
	}
}

void MixedParticlesDemo::reset()
{
	float s = 1.f;
	float r = 0.1f;
	float rs = 0.01f;
	float density = 100.f;

	r /= RESOLUTION_FACTOR;
	rs /= RESOLUTION_FACTOR;

	for(int i=0; i<m_nL; i++)
	{
		m_pos[i] = make_float4( getRandom( -s, s ), getRandom( -s, s ), 0.f );
		float mr = m_pos[i].w = getRandom( r/8.f, r );
		m_vel[i] = make_float4( 0, 0, 0, mr*mr*PI*density );
		m_force[i] = make_float4( 0.f );

		if( i%4 == 0 ) m_vel[i].w = FLT_MAX;
	}

	{
		float4* p = m_posMapped.getPtr<float4>();
		float4* v = m_velMapped.getPtr<float4>();
		float4* f = new float4[m_nS];
		for(int i=0; i<m_nS; i++)
		{
			p[i] = make_float4( getRandom( -s, s ), getRandom( -s, s ), 0.f );
			p[i].w = rs;
			v[i] = make_float4( 0, 0, 0, rs*rs*PI*density );
			f[i] = make_float4(0.f);
		}

		m_forceSD.write( f, m_nS );
		m_forceIntD.write( f, m_nS );
		DeviceUtils::waitForCompletion( m_deviceData );
		delete [] f;
	}

	if( !m_ugData )
	{
		int nCells;
		if( m_ugData )
		{
			nCells = m_ugData->m_nCells.x*m_ugData->m_nCells.y*m_ugData->m_nCells.z;
			m_gridMapped.unmap( *m_ugData->m_gridData, nCells );
			m_gridCounterMapped.unmap( *m_ugData->m_counter, nCells );
			delete m_ugData;
		}
		UniformGrid<TYPE_CL>::Config cfg;
		cfg.m_max = make_float4(s, s, 0, 0 ) + rs;
		cfg.m_min = make_float4(-s, -s, 0, 0 ) - rs;
		cfg.m_spacing = rs*2.f;

		m_ugData = UniformGrid<TYPE_CL>::allocate( m_deviceData, cfg, true );
		nCells = m_ugData->m_nCells.x*m_ugData->m_nCells.y*m_ugData->m_nCells.z;
		m_gridMapped.map( m_deviceData, *m_ugData->m_gridData, nCells );
		m_gridCounterMapped.map( m_deviceData, *m_ugData->m_counter, nCells );
		DeviceUtils::waitForCompletion( m_deviceData );
	}
}

#include <Demos/MixedParticlesJobs.inl>

void MixedParticlesDemo::step( float dt )
{
	dt *= 0.0125f;
	dt /= 20.f;

	float e = 0.85f;
	float4 g = make_float4(0.f, -9.8f, 0.f, 0.f) * 0.5f;

	if( m_stepCount == 0 )
	{
		UniformGrid<TYPE_CL>::execute( m_ugData, m_posSD, m_nS );
		DeviceUtils::waitForCompletion( m_deviceData );
	}

	ThreadPool& threadPool = *m_threads;

	const int PAIR_CAPACITY = m_nL * 15;
	Pair32* pairs = new Pair32[PAIR_CAPACITY];
	float4* forceBuffer = new float4[ m_nS*3 ];
	{
		for(int i=0; i<3; i++)
			forceBuffer[i*m_nS] = make_float4(-1.f);
	}

	ConstBuffer cb;
	{
		cb.m_g = g;
		cb.m_numParticles = m_nS;
		cb.m_dt = dt;
		cb.m_scale = 1.f;
		cb.m_e = e;
		cb.m_nCells = m_ugData->m_nCells;
		cb.m_spaceMin = m_ugData->m_min;
		cb.m_gridScale = m_ugData->m_gridScale;
	}

	int cpuTaskLock = 0;

//	float4* posS = m_posMapped.getPtr<float4>();
//	float4* velS = m_velMapped.getPtr<float4>();
	float4* posS = new float4[m_nS];
	float4* velS = new float4[m_nS];
	memcpy( posS, m_posMapped.getPtr<float4>(), sizeof(float4)*m_nS );
	memcpy( velS, m_velMapped.getPtr<float4>(), sizeof(float4)*m_nS );


	float4* forceS = m_forceMapped.getPtr<float4>();

	GPUCollideTask* gCollideTask = new GPUCollideTask( m_deviceData, &m_posSD, &m_velSD, &m_forceSD, m_nS, &m_constBuffer, &cb, &m_collideGridKernel, m_ugData, 
		(COPY_NATIVE_GPU)? &m_copyPosVelKernel:0, &m_posSNative, &m_velSNative);
	CPUAabbBuildTask* cAabbBuildTask = new CPUAabbBuildTask( m_pos, m_nL, m_aabb, m_aabbUint );

	int nInAJob = m_nL/3;
	CPUInteractSubTask* cInteractTask0 = new CPUInteractSubTask( m_pos, m_vel, m_force, nInAJob, 
		posS, velS, &forceBuffer[0*m_nS], m_nS,
		dt, e, &m_gridCounterMapped, &m_gridMapped, m_ugData );

	CPUInteractSubTask* cInteractTask1 = new CPUInteractSubTask( m_pos+nInAJob, m_vel+nInAJob, m_force+nInAJob, nInAJob, 
		posS, velS, &forceBuffer[1*m_nS], m_nS,
		dt, e, &m_gridCounterMapped, &m_gridMapped, m_ugData );

	CPUInteractSubTask* cInteractTask2 = new CPUInteractSubTask( m_pos+2*nInAJob, m_vel+2*nInAJob, m_force+2*nInAJob, m_nL-2*nInAJob, 
		posS, velS, &forceBuffer[2*m_nS], m_nS,
		dt, e, &m_gridCounterMapped, &m_gridMapped, m_ugData );

	CPUMergeTask* cMergeTask = new CPUMergeTask( forceS, forceBuffer, m_nS, m_nS, 3 );
	{
		cAabbBuildTask->m_taskLock = &cpuTaskLock;
		cInteractTask0->m_taskLock = &cpuTaskLock;
		cInteractTask1->m_taskLock = &cpuTaskLock;
		cInteractTask2->m_taskLock = &cpuTaskLock;
		cMergeTask->m_taskLock = &cpuTaskLock;

		threadPool.pushBack( gCollideTask );
		threadPool.pushBack( cAabbBuildTask );
		threadPool.pushBack( cInteractTask0 );
		threadPool.pushBack( cInteractTask1 );
		threadPool.pushBack( cInteractTask2 );
//		threadPool.start(true);
//		threadPool.wait();
		threadPool.pushBack( cMergeTask );
		threadPool.start(true);
		threadPool.wait();
	}

	//-------------------------
	//	sync
	//-------------------------

	CPUCollideIntegrateTask* cCollideIntegrateTask = new CPUCollideIntegrateTask( m_pos, m_vel, m_force, m_nL, dt, e, g, m_planes, pairs, PAIR_CAPACITY, m_aabbUint );
	GPUIntegrateBuildTask* gIntegrateBuildTask = new GPUIntegrateBuildTask( m_deviceData, &m_posSD, &m_velSD, &m_forceSD, &m_forceIntD, m_nS, &m_constBuffer, &cb, &m_integrateKernel, m_ugData,
		(COPY_NATIVE_GPU)? &m_copyPosVelKernel:0, &m_posSNative, &m_velSNative);

	{
		threadPool.pushBack( cCollideIntegrateTask );
		threadPool.pushBack( gIntegrateBuildTask );
		threadPool.start(false);
		threadPool.wait();
	}

	delete [] forceBuffer;
	delete [] posS;
	delete [] velS;
	delete [] pairs;

	{
		m_nTxtLines = 0;
		{
			sprintf_s(m_txtBuffer[m_nTxtLines++], LINE_CAPACITY, "S:%d, L:%d", m_nS, m_nL);
		}
	}

	if( m_stepCount%1000 == 999 ) reset();

	if(1) return;

	{
		UniformGrid<TYPE_HOST>::Config cfg;
		{
			cfg.m_max = m_ugData->m_max;
			cfg.m_min = m_ugData->m_min;
			cfg.m_spacing = m_posMapped.getPtr<float4>()[0].w*2.f;
		}

//		Buffer<float4> hostPos;
//		hostPos.m_deviceData = m_deviceHost;
//		hostPos.m_ptr = m_posMapped.getPtr<float4>();

		Buffer<float4> hostPos( m_deviceHost, m_nS );
		m_posSD.read( hostPos.m_ptr, m_nS );
		DeviceUtils::waitForCompletion( m_deviceData );


		m_hostUgData = UniformGrid<TYPE_HOST>::allocate( m_deviceHost, cfg );

		UniformGrid<TYPE_HOST>::execute( m_hostUgData, hostPos, m_nS );

		
		{
			if(0)
			for(int i=0; i<m_ugData->getNCells(); i++)
			{
				u32* d = m_gridCounterMapped.getPtr<u32>();
				u32* h =((u32*)m_hostUgData->m_counter->m_ptr);
				ADLASSERT( d[i] == h[i] );
			}
			//((float4*)m_posMapped.m_ptr)[4918]
			//((float4*)m_posMapped.m_ptr)[7476]
			m_ugData->m_counter->write( m_hostUgData->m_counter->m_ptr, m_ugData->getNCells() );
			m_ugData->m_gridData->write( m_hostUgData->m_gridData->m_ptr, m_ugData->getNCells()*MAX_IDX_PER_GRID );
			DeviceUtils::waitForCompletion( m_deviceData );

//			memcpy( m_gridCounterMapped.getPtr<u32>(), m_hostUgData->m_counter->m_ptr, sizeof(u32)*m_ugData->getNCells() );
//			memcpy( m_gridMapped.getPtr<u32>(), m_hostUgData->m_gridData->m_ptr, sizeof(u32)*m_ugData->getNCells()*MAX_IDX_PER_GRID );
		}


		UniformGrid<TYPE_HOST>::deallocate( m_hostUgData );

//		hostPos.m_deviceData = 0;
	}
}

void MixedParticlesDemo::render()
{
	{
		float4* p = new float4[m_nL];
		float4* c = new float4[m_nL];
		float2* r = new float2[m_nL];

		for(int i=0; i<m_nL; i++)
		{
			p[i] = m_pos[i];
			c[i] = make_float4(0.f, 1.f, 0.f, 1.f);
			r[i].x = m_pos[i].w;
		}

		pxDrawPointSprite( p, c, r, m_nL );

		delete [] p;
		delete [] c;
		delete [] r;
	}


	{
		float4* p = new float4[m_nS];
		float4* c = new float4[m_nS];
		float2* r = new float2[m_nS];

		for(int i=0; i<m_nS; i++)
		{
			p[i] = m_posMapped.getPtr<float4>()[i];
			c[i] = make_float4(0.f, 0.f, 1.f, 1.f);
			r[i].x = p[i].w;
		}

		pxDrawPointSprite( p, c, r, m_nS );

		delete [] p;
		delete [] c;
		delete [] r;
	}
}

void MixedParticlesDemo::renderPre()
{
	if( !ENABLE_POST_EFFECT ) return;

	{	//	prepare g buffer
		m_deviceRender->m_context->OMGetRenderTargets( 1, &m_rtv, &m_dsv );

		float ClearColor[4] = { m_backgroundColor.x, m_backgroundColor.y, m_backgroundColor.z };

		ID3D11RenderTargetView* targets[] = { m_colorRT.m_renderTarget };

		m_deviceRender->m_context->OMSetRenderTargets( 1, targets, m_dsv );
		m_deviceRender->m_context->ClearRenderTargetView( m_colorRT.m_renderTarget, ClearColor );
		m_deviceRender->m_context->ClearDepthStencilView( m_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0 );
	}

}

void MixedParticlesDemo::renderPost()
{
	if( !ENABLE_POST_EFFECT ) return;


	m_deviceRender->m_context->OMSetRenderTargets( 0, 0, 0 );

	Buffer<int> colorBuffer; colorBuffer.m_srv = m_colorRT.m_srv;
	Buffer<int> depthBuffer; depthBuffer.m_srv = g_depthStencil.m_srv;

	RConstBuffer cb;
	{
		cb.m_width = g_wWidth;
		cb.m_height = g_wHeight;
	}

	{	//	run post effect
		BufferInfo bInfo[] = { BufferInfo( &colorBuffer, true ), BufferInfo( &depthBuffer, true ), 
			BufferInfo( &m_renderBuffer ) };

		Launcher launcher( m_deviceRender, &m_postProcessKernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( m_rConstBuffer, cb );
		launcher.launch1D( g_wWidth*g_wHeight );
	}

	resolveSrv( &m_renderBuffer.m_srv );
}

void MixedParticlesDemo::resolveSrv( void** srv )
{
	//	release for the renderPre
	m_rtv->Release();
	m_dsv->Release();

	{
		float ClearColor[4] = { 0.f, 0.f, 0.f, 1.0f };
		ID3D11RenderTargetView* aRTViews[ 1 ] = { m_rtv };
		m_deviceRender->m_context->OMSetRenderTargets( 1, aRTViews, m_dsv );
		m_deviceRender->m_context->ClearDepthStencilView( m_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0 );
		m_deviceRender->m_context->ClearRenderTargetView( m_rtv, ClearColor );

		m_deviceRender->m_context->PSSetShaderResources( 0, 1, (ID3D11ShaderResourceView**)srv );

		//	render to screen
		renderFullQuad( m_deviceRender, &g_bufferToRTPixelShader, make_float4((float)g_wWidth, (float)g_wHeight, 0,0 ) );

		ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
		m_deviceRender->m_context->PSSetShaderResources( 0, 1, ppSRVNULL );
	}

//	g_defaultVertexShader = m_gVShader;
//	g_defaultPixelShader = m_gPShader;
}
