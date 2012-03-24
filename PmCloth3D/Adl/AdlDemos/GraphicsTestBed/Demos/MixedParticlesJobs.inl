#include <Common/Base/SyncObjects.h>

struct DummyTask : public ThreadPool::Task
{
	typedef Launcher::BufferInfo BufferInfo;

	u16 getType(){ return 1; }

	void run(int tIdx)
	{
	}
};

struct CPUCollideIntegrateTask : public ThreadPool::Task
{
	CPUCollideIntegrateTask(){}
	CPUCollideIntegrateTask(float4* pos, float4* vel, float4* force, int n,
		float dt, float e, const float4& g, float4* boundaries, Pair32* pairs, int pairCapacity, AabbUint* aabbUint)
		: m_pos(pos), m_vel(vel), m_force(force), m_n(n), m_dt(dt), m_e(e), m_g(g),
		m_boundaries(boundaries), m_pairs(pairs), m_pairCapacity(pairCapacity), m_aabbUint(aabbUint){}

	u16 getType(){ return 1; }

	void run(int tIdx)
	{
		//	CPU broadphase
		{
			SweepBroadphase bpsp;
			int nPairs = bpsp.getPair( m_aabbUint, m_n, m_pairs, m_pairCapacity );

			for(int i=0; i<nPairs; i++)
			{
				int aIdx = m_pairs[i].m_a;
				int bIdx = m_pairs[i].m_b;

				float4 f = calcForce( m_pos[aIdx], m_pos[bIdx], m_vel[aIdx], m_vel[bIdx], m_pos[aIdx].w, m_pos[bIdx].w, m_vel[aIdx].w, m_vel[bIdx].w,
					m_dt, m_e );

				m_force[aIdx] += f;
				m_force[bIdx] -= f;
			}
		}

		//	boundary
		calcBoundary( m_pos, m_vel, m_force, m_n, m_dt, m_e, m_boundaries, 4 );

		//	CPU integration
		integration( m_pos, m_vel, m_force, m_n, m_dt, m_g );
	}

	float4* m_pos;
	float4* m_vel;
	float4* m_force;
	int m_n;
	
	float m_dt;
	float m_e;
	float4 m_g;

	float4* m_boundaries;

	Pair32* m_pairs;
	int m_pairCapacity;
	AabbUint* m_aabbUint;
};

struct GPUIntegrateBuildTask : public ThreadPool::Task
{
	typedef Launcher::BufferInfo BufferInfo;

	GPUIntegrateBuildTask(){}
	GPUIntegrateBuildTask(const Device* device, Buffer<float4>* pos, Buffer<float4>* vel, Buffer<float4>* force,
		Buffer<float4>* forceInteraction, int n, 
		Buffer<MixedParticlesDemo::ConstBuffer>* constBuffer,
		MixedParticlesDemo::ConstBuffer* constData, 
		Kernel* integrateKernel,
		UniformGrid<TYPE_CL>::Data* ugData,
		Kernel* copyPosVelKernel=0, Buffer<float4>* posNative = 0, Buffer<float4>* velNative = 0)
		: m_deviceData(device), m_pos(pos), m_vel(vel), m_force(force), m_forceInteraction(forceInteraction),
		m_n(n), m_constBuffer(constBuffer), m_constData(constData),
		m_integrateKernel(integrateKernel), m_ugData(ugData),
		m_copyPosVelKernel( copyPosVelKernel ),
		m_posNative( posNative ), m_velNative( velNative ){}

	u16 getType(){ return 0; }

	void run(int tIdx)
	{
		bool doCopy = ( m_copyPosVelKernel )? true:false;
		Buffer<float4>* p = (doCopy)? m_posNative : m_pos;
		Buffer<float4>* v = (doCopy)? m_velNative : m_vel;

		//	GPU integrate
		{
			BufferInfo bInfo[] = { BufferInfo( p ), 
				BufferInfo( v ), BufferInfo( m_force ), BufferInfo( m_forceInteraction ) };

			Launcher launcher( m_deviceData, m_integrateKernel );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( *m_constBuffer, *m_constData );
			launcher.launch1D( m_n );
		}

		//	GPU grid construction
		{
			UniformGrid<TYPE_CL>::execute( m_ugData, *p, m_n );
		}

		if( doCopy )
		{
			BufferInfo bInfo[] = { BufferInfo( m_posNative ), BufferInfo( m_velNative ),
				BufferInfo( m_pos ), BufferInfo( m_vel ) };

			Launcher launcher( m_deviceData, m_copyPosVelKernel );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( *m_constBuffer, *m_constData );
			launcher.launch1D( m_n );
		}

		DeviceUtils::waitForCompletion( m_deviceData );
	}

	const Device* m_deviceData;
	Buffer<float4>* m_pos;
	Buffer<float4>* m_vel;
	Buffer<float4>* m_force;
	Buffer<float4>* m_forceInteraction;
	int m_n;
	Buffer<MixedParticlesDemo::ConstBuffer>* m_constBuffer;
	MixedParticlesDemo::ConstBuffer* m_constData;

	Kernel* m_integrateKernel;
	UniformGrid<TYPE_CL>::Data* m_ugData;

	Kernel* m_copyPosVelKernel;
	Buffer<float4>* m_posNative;
	Buffer<float4>* m_velNative;
};

struct GPUCollideTask : public ThreadPool::Task
{
	typedef Launcher::BufferInfo BufferInfo;

	GPUCollideTask(){}
	GPUCollideTask(const Device* device, Buffer<float4>* pos, Buffer<float4>* vel, Buffer<float4>* force,
		int n, 
		Buffer<MixedParticlesDemo::ConstBuffer>* constBuffer,
		MixedParticlesDemo::ConstBuffer* constData, 
		Kernel* collideGridKernel,
		UniformGrid<TYPE_CL>::Data* ugData,
		Kernel* copyPosVelKernel=0, Buffer<float4>* posNative = 0, Buffer<float4>* velNative = 0)
		: m_deviceData(device), m_pos(pos), m_vel(vel), m_force(force), 
		m_n(n), m_constBuffer(constBuffer), m_constData(constData),
		m_collideGridKernel(collideGridKernel), m_ugData(ugData), 
		m_copyPosVelKernel( copyPosVelKernel ),
		m_posNative( posNative ), m_velNative( velNative ){}

	u16 getType(){ return 0; }

	void run(int tIdx)
	{
		bool doCopy = ( m_copyPosVelKernel )? true:false;

		if( doCopy )
		{
			BufferInfo bInfo[] = { BufferInfo( m_pos ), BufferInfo( m_vel ), 
				BufferInfo( m_posNative ), BufferInfo( m_velNative ) };

			Launcher launcher( m_deviceData, m_copyPosVelKernel );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( *m_constBuffer, *m_constData );
			launcher.launch1D( m_n );
		}

		{
			Buffer<float4>* p = (doCopy)? m_posNative : m_pos;
			Buffer<float4>* v = (doCopy)? m_velNative : m_vel;

			BufferInfo bInfo[] = { BufferInfo( p ), BufferInfo( v ), 
				BufferInfo( m_ugData->m_gridData ), BufferInfo( m_ugData->m_counter ),
				BufferInfo( m_force ) };

			Launcher launcher( m_deviceData, m_collideGridKernel );
			launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
			launcher.setConst( *m_constBuffer, *m_constData );
			launcher.launch1D( m_n );
		}

		DeviceUtils::waitForCompletion( m_deviceData );
	}

	const Device* m_deviceData;
	Buffer<float4>* m_pos;
	Buffer<float4>* m_vel;
	Buffer<float4>* m_force;
	int m_n;
	Buffer<MixedParticlesDemo::ConstBuffer>* m_constBuffer;
	MixedParticlesDemo::ConstBuffer* m_constData;

	Kernel* m_collideGridKernel;
	UniformGrid<TYPE_CL>::Data* m_ugData;

	Kernel* m_copyPosVelKernel;
	Buffer<float4>* m_posNative;
	Buffer<float4>* m_velNative;
};

struct CPUAabbBuildTask : public ThreadPool::Task
{
	typedef Launcher::BufferInfo BufferInfo;

	CPUAabbBuildTask(){}
	CPUAabbBuildTask( float4* pos, int n, Aabb* aabb, AabbUint* aabbUint )
		: m_pos( pos ), m_n( n ), m_aabb( aabb ), m_aabbUint( aabbUint ) {}

	u16 getType(){ return 2; }

	void run(int tIdx)
	{
		//	build Aabb
		for(int i=0; i<m_n; i++)
		{
			float r = m_pos[i].w;
			const float4& c = m_pos[i];
			m_aabb[i].m_max = c + make_float4(r);
			m_aabb[i].m_min = c - make_float4(r);
		}

		//	convert Aabb
		{
			Aabb space;
			space.m_max = make_float4(1.5f,1.5f,0.1f);
			space.m_min = make_float4(-1.5f,-1.5f,-0.1f);
			convertToAabbUint( m_aabb, m_aabbUint, m_n, space );
		}

		atomAdd( m_taskLock, 1 );
	}

	float4* m_pos;
	int m_n;
	Aabb* m_aabb;
	AabbUint* m_aabbUint;
	int* m_taskLock;

};

struct CPUInteractTask : public ThreadPool::Task
{
	typedef Launcher::BufferInfo BufferInfo;

	CPUInteractTask(){}
	CPUInteractTask( float4* pos0, float4* vel0, float4* force0, int n0,
		float4* pos1, float4* vel1, float4* force1, int n1,
		float dt, float e, MapBuffer* gridCounterMapped, MapBuffer* gridMapped,
		UniformGrid<TYPE_CL>::Data* ugData)
		: m_pos0(pos0), m_vel0(vel0), m_force0(force0), m_n0(n0),
		m_pos1(pos1), m_vel1(vel1), m_force1(force1), m_n1(n1),
		m_dt(dt), m_e(e), m_gridCounterMapped( gridCounterMapped ), m_gridMapped( gridMapped ),
		m_ugData( ugData ) {}

	u16 getType(){ return 1; }

	void run(int tIdx)
	{
		volatile int* lock = m_taskLock;
		while( *lock < 1 ){}

		calcInteractions( m_pos0, m_vel0, m_force0, m_n0, m_pos1, m_vel1, m_force1, m_n1, 
			m_dt, m_e, m_ugData, m_gridCounterMapped, m_gridMapped );
	}

	float4* m_pos0;
	float4* m_vel0;
	float4* m_force0;
	int m_n0;
	
	float4* m_pos1;
	float4* m_vel1;
	float4* m_force1;
	int m_n1;

	float m_dt;
	float m_e;

	MapBuffer* m_gridCounterMapped;
	MapBuffer* m_gridMapped;

	UniformGrid<TYPE_CL>::Data* m_ugData;

	int* m_taskLock;
};

struct CPUInteractSubTask : public ThreadPool::Task
{
	typedef Launcher::BufferInfo BufferInfo;

	CPUInteractSubTask(){}
	CPUInteractSubTask( float4* pos0, float4* vel0, float4* force0, int n0,
		float4* pos1, float4* vel1, float4* force1, int n1,
		float dt, float e, MapBuffer* gridCounterMapped, MapBuffer* gridMapped,
		UniformGrid<TYPE_CL>::Data* ugData)
		: m_pos0(pos0), m_vel0(vel0), m_force0(force0), m_n0(n0),
		m_pos1(pos1), m_vel1(vel1), m_force1(force1), m_n1(n1),
		m_dt(dt), m_e(e), m_gridCounterMapped( gridCounterMapped ), m_gridMapped( gridMapped ),
		m_ugData( ugData ) {}

	u16 getType(){ return 1; }

	void run(int tIdx)
	{
		if( m_force1[0].z == -1.f )
		{
			for(int i=0; i<m_n1; i++) m_force1[i] = make_float4(0.f);
		}

		volatile int* lock = m_taskLock;
		while( *lock < 1 ){}

		calcInteractions( m_pos0, m_vel0, m_force0, m_n0, m_pos1, m_vel1, m_force1, m_n1, 
			m_dt, m_e, m_ugData, m_gridCounterMapped, m_gridMapped );

		atomAdd( m_taskLock, 1 );
	}

	float4* m_pos0;
	float4* m_vel0;
	float4* m_force0;
	int m_n0;
	
	float4* m_pos1;
	float4* m_vel1;
	float4* m_force1;
	int m_n1;

	float m_dt;
	float m_e;

	MapBuffer* m_gridCounterMapped;
	MapBuffer* m_gridMapped;

	UniformGrid<TYPE_CL>::Data* m_ugData;

	int* m_taskLock;
};


struct CPUMergeTask : public ThreadPool::Task
{
	typedef Launcher::BufferInfo BufferInfo;

	CPUMergeTask(){}
	CPUMergeTask(float4* dst, float4* src, int offset, int n, int nSets)
		: m_dst(dst), m_src(src), m_offset(offset), m_n(n), m_nSets(nSets){}

	u16 getType(){ return 2; }

	void run(int tIdx)
	{
		volatile int* lock = m_taskLock;
		while( *lock < 4 ){}

		bool initialized = false;

		for(int ii=0; ii<m_nSets; ii++)
		{
			float4* f = &m_src[m_offset*ii];
			if( f[0].z == -1 ) continue;
			if( initialized )
			{
				for(int i=0; i<m_n; i++)
				{
					m_dst[i] += f[i];
				}
			}
			else
			{
				for(int i=0; i<m_n; i++)
				{
					m_dst[i] = f[i];
				}
			}

			initialized = true;
		}
	}
	float4* m_dst;
	float4* m_src;
	int m_offset;
	int m_n;
	int m_nSets;
	int* m_taskLock;
};

