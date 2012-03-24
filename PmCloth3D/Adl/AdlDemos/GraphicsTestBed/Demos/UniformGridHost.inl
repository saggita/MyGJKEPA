template<>
class UniformGrid<TYPE_HOST> : public UniformGridBase
{
	public:
		typedef Launcher::BufferInfo BufferInfo;

		struct Data
		{
			float4 m_max;
			float4 m_min;
			int4 m_nCells;
			float m_gridScale;

			const Device* m_device;

			Buffer<u32>* m_counter;
			Buffer<u32>* m_gridData;

			int getNCells() const { return m_nCells.x*m_nCells.y*m_nCells.z; }
		};

		static
		Data* allocate( const Device* device, const Config& cfg, bool allocateZeroCopyBuffer = false )
		{
			ADLASSERT( device->m_type == TYPE_HOST );

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

			data->m_counter = new Buffer<u32>( device, nCells );
			data->m_gridData = new Buffer<u32>( device, nCells*MAX_IDX_PER_GRID );

			return data;
		}

		static
		void deallocate(Data* data)
		{
			delete data->m_counter;
			delete data->m_gridData;

			delete data;
		}

		static
		void execute( Data* data, Buffer<float4>& pos, int n )
		{
			ADLASSERT( pos.getType() == TYPE_HOST );
			HostBuffer<u32>& gridG = (HostBuffer<u32>&)*data->m_gridData;
			HostBuffer<u32>& gridCounterG = (HostBuffer<u32>&)*data->m_counter;
			HostBuffer<float4>& gPosIn = (HostBuffer<float4>&)pos;

			int nCells = data->m_nCells.x*data->m_nCells.y*data->m_nCells.z;
			{	//	clear grid
				for(int i=0; i<nCells; i++)
				{
					gridCounterG[i] = 0;
				}
			}

			{	//	fill
				Data& cb = *data;

				for(int gIdx=0; gIdx<n; gIdx++)
				{
					float4 iPos = gPosIn[gIdx];
					int4 gridCrd = ugConvertToGridCrd( iPos-cb.m_min, cb.m_gridScale );

					if( gridCrd.x < 0 || gridCrd.x >= cb.m_nCells.x 
						|| gridCrd.y < 0 || gridCrd.y >= cb.m_nCells.y
						|| gridCrd.z < 0 || gridCrd.z >= cb.m_nCells.z ) continue;
	
					int gridIdx = ugGridCrdToGridIdx( gridCrd, cb.m_nCells.x, cb.m_nCells.y, cb.m_nCells.z );

					int count = gridCounterG[gridIdx]++;//atom_add(&gridCounterG[gridIdx], 1);

					if( count < MAX_IDX_PER_GRID )
					{
						gridG[ gridIdx*MAX_IDX_PER_GRID + count ] = gIdx;
					}
				}
			}

			{	//	assert
				int total = 0;
				for(int i=0; i<nCells; i++)
				{
					total += gridCounterG[i];
				}
				ADLASSERT( total <= n );
			}
		}


		__inline
		static int4 ugConvertToGridCrd(const float4& pos, float gridScale)
		{
			int4 g;
			g.x = (int)floor(pos.x*gridScale);
			g.y = (int)floor(pos.y*gridScale);
			g.z = (int)floor(pos.z*gridScale);
			return g;
		}

		__inline
		static int ugGridCrdToGridIdx(const int4& g, int nCellX, int nCellY, int nCellZ)
		{
			return g.x+g.y*nCellX+g.z*nCellX*nCellY;
		}
};


