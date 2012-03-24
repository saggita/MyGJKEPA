struct MapBuffer
{
	MapBuffer(): m_device(0), m_ptr(0){}
	~MapBuffer(){ ADLASSERT( m_ptr==0 ); }
	template<typename T>
	T* getPtr() { return (T*)m_ptr; }

	template<typename T>
	void map(const Device* device, const Buffer<T>& buffer, int nElems);

	template<typename T>
	void unmap(const Buffer<T>& buffer, int nElems);

	const Device* m_device;
	void* m_ptr;
};



template<typename T>
void MapBuffer::map(const Device* device, const Buffer<T>& buffer, int nElems)
{
	ADLASSERT( m_device == 0 );
	ADLASSERT( device->m_type == buffer.getType() );

	m_device = device;

	switch( m_device->m_type )
	{
	case TYPE_CL:
		{
			const DeviceCL* dcl = (const DeviceCL*)m_device;
			cl_int e;
			m_ptr = clEnqueueMapBuffer( dcl->m_commandQueue, buffer.m_ptr, false, CL_MAP_READ | CL_MAP_WRITE, 0, sizeof(T)*nElems, 0,0,0,&e );
			ADLASSERT( e == CL_SUCCESS );
		}
		break;
	default:
		break;
	};
}

template<typename T>
void MapBuffer::unmap(const Buffer<T>& buffer, int nElems)
{
	ADLASSERT( m_device == 0 );
	ADLASSERT( m_ptr );
	ADLASSERT( m_device->m_type == buffer.getType() );

	switch( m_device->m_type )
	{
	case TYPE_CL:
		{
			const DeviceCL* dcl = (const DeviceCL*)m_device;
			cl_int e;
			clEnqueueUnmapMemObject( dcl->m_commandQueue, buffer.m_ptr, m_ptr, 0,0,0 );
			ADLASSERT( e == CL_SUCCESS );
		}
		break;
	default:
		break;
	};
}

