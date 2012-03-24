
#define PATH "Demos\\UniformGridKernels"
#define KERNEL0 "GridClearKernel"
#define KERNEL1 "GridConstructionKernel"

template<DeviceType TYPE>
typename UniformGrid<TYPE>::Data* UniformGrid<TYPE>::allocate( const Device* device, const Config& cfg, bool allocateZeroCopyBuffer )
{
	Data* data = new Data;
	data->m_device = device;

	int nCells;
	{
		float cellSize = cfg.m_spacing;
		float4 extent = cfg.m_max - cfg.m_min;
		int nx = int(extent.x/cellSize)+1;
		int ny = int(extent.y/cellSize)+1;
		int nz = int(extent.z/cellSize)+1;

		data->m_gridScale = 1.f/(cellSize);
		data->m_nCells = make_int4(nx, ny, nz);
		data->m_max = cfg.m_max;
		data->m_min = cfg.m_min;

		nCells = nx*ny*nz;
	}

	BufferBase::BufferType type = (allocateZeroCopyBuffer)? BufferBase::BUFFER_ZERO_COPY: BufferBase::BUFFER;
	data->m_counter = new Buffer<u32>( device, nCells, type );
	data->m_gridData = new Buffer<u32>( device, nCells*MAX_IDX_PER_GRID, type );
	data->m_constBuffer = new Buffer<ConstData>( device, 1, BufferBase::BUFFER_CONST );

//	data->m_clearKernel = KernelManager::query( device, PATH, KERNEL0, 0, 0 );
//	data->m_gridConstructionKernel = KernelManager::query( device, PATH, KERNEL1, 0, 0 );
	{
		data->m_clearKernel = new Kernel;
		data->m_gridConstructionKernel = new Kernel;

		const char *option = "-I ..\\";
		KernelBuilder<TYPE> builder;
		builder.setFromFile( device, PATH, option, true );
		builder.createKernel( KERNEL0, *data->m_clearKernel );
		builder.createKernel( KERNEL1, *data->m_gridConstructionKernel );
	}

	return data;
}

template<DeviceType TYPE>
void UniformGrid<TYPE>::deallocate(Data* data)
{
	delete data->m_counter;
	delete data->m_gridData;
	delete data->m_constBuffer;

	KernelBuilder<TYPE>::deleteKernel( *data->m_clearKernel );
	KernelBuilder<TYPE>::deleteKernel( *data->m_gridConstructionKernel );

	delete data->m_clearKernel;
	delete data->m_gridConstructionKernel;

	delete data;
}


template<DeviceType TYPE>
void UniformGrid<TYPE>::execute( Data* data, Buffer<float4>& pos, int n )
{
	ADLASSERT( pos.getType() == TYPE );

	ConstData cb;
	{
		cb.m_max = data->m_max;
		cb.m_min = data->m_min;
		cb.m_nCells = data->m_nCells;
		cb.m_gridScale = data->m_gridScale;
		cb.m_n = n;
	}

	{
		int size = data->m_nCells.x*data->m_nCells.y*data->m_nCells.z;

		BufferInfo bInfo[] = { BufferInfo( data->m_counter ) };

		Launcher launcher( data->m_device, data->m_clearKernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( *data->m_constBuffer, cb );
		launcher.launch1D( size );
	}

	{
		BufferInfo bInfo[] = { BufferInfo( &pos ), 
			BufferInfo( data->m_gridData ), BufferInfo( data->m_counter ) };

		Launcher launcher( data->m_device, data->m_gridConstructionKernel );
		launcher.setBuffers( bInfo, sizeof(bInfo)/sizeof(Launcher::BufferInfo) );
		launcher.setConst( *data->m_constBuffer, cb );
		launcher.launch1D( n );
	}
}

#undef PATH
#undef KERNEL0
#undef KERNEL1
