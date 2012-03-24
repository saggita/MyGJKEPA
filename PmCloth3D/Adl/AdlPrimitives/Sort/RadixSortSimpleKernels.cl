#pragma OPENCL EXTENSION cl_amd_printf : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable

typedef unsigned int u32;
#define GET_GROUP_IDX get_group_id(0)
#define GET_LOCAL_IDX get_local_id(0)
#define GET_GLOBAL_IDX get_global_id(0)
#define GET_GROUP_SIZE get_local_size(0)
#define GROUP_LDS_BARRIER barrier(CLK_LOCAL_MEM_FENCE)
#define AtomInc(x) atom_inc(&(x))
#define AtomInc1(x, out) out = atom_inc(&(x))


#define WG_SIZE 128
#define NUM_PER_WI 4


typedef struct
{
	u32 m_key; 
	u32 m_value;
}SortData;


typedef struct
{
	u32 m_startBit;
	u32 m_numGroups;
	u32 m_padding[2];
} ConstBuffer;


__kernel
__attribute__((reqd_work_group_size(WG_SIZE,1,1)))
void LocalCountKernel(__global SortData* sortData, 
						__global u32* ldsHistogramOut,
						ConstBuffer cb)
{
	__local u32 ldsHistogram[16][256];

	int lIdx = GET_LOCAL_IDX;
	int gIdx = GET_GLOBAL_IDX;
	
	for(int i=0; i<16; i++)
	{
		ldsHistogram[i][lIdx] = 0.f;
		ldsHistogram[i][lIdx+128] = 0.f;
	}
	
	GROUP_LDS_BARRIER;
	
	SortData datas[NUM_PER_WI];
	datas[0] = sortData[gIdx*NUM_PER_WI+0];
	datas[1] = sortData[gIdx*NUM_PER_WI+1];
	datas[2] = sortData[gIdx*NUM_PER_WI+2];
	datas[3] = sortData[gIdx*NUM_PER_WI+3];

	datas[0].m_key = (datas[0].m_key >> cb.m_startBit) & 0xff;
	datas[1].m_key = (datas[1].m_key >> cb.m_startBit) & 0xff;
	datas[2].m_key = (datas[2].m_key >> cb.m_startBit) & 0xff;
	datas[3].m_key = (datas[3].m_key >> cb.m_startBit) & 0xff;

	int tableIdx = lIdx%16;
	
	AtomInc(ldsHistogram[tableIdx][datas[0].m_key]);
	AtomInc(ldsHistogram[tableIdx][datas[1].m_key]);
	AtomInc(ldsHistogram[tableIdx][datas[2].m_key]);
	AtomInc(ldsHistogram[tableIdx][datas[3].m_key]);

	GROUP_LDS_BARRIER;
	
	u32 sum0, sum1;
	sum0 = sum1 = 0;
	for(int i=0; i<16; i++)
	{
		sum0 += ldsHistogram[i][lIdx];
		sum1 += ldsHistogram[i][lIdx+128];
	}

	ldsHistogramOut[lIdx*cb.m_numGroups+GET_GROUP_IDX] = sum0;
	ldsHistogramOut[(lIdx+128)*cb.m_numGroups+GET_GROUP_IDX] = sum1;
}

__kernel
__attribute__((reqd_work_group_size(WG_SIZE,1,1)))
void ScatterKernel(__global SortData* sortData,
					__global SortData* sortDataOut,
					__global u32* scannedHistogram, 
					ConstBuffer cb)
{
	__local u32 ldsCurrentLocation[256];

	int lIdx = GET_LOCAL_IDX;
	int gIdx = GET_GLOBAL_IDX;
	
	{
		ldsCurrentLocation[lIdx] = scannedHistogram[lIdx*cb.m_numGroups+GET_GROUP_IDX];
		ldsCurrentLocation[lIdx+128] = scannedHistogram[(lIdx+128)*cb.m_numGroups+GET_GROUP_IDX];
	}

	GROUP_LDS_BARRIER;
	
	SortData datas[NUM_PER_WI];
	int keys[NUM_PER_WI];
	datas[0] = sortData[gIdx*NUM_PER_WI+0];
	datas[1] = sortData[gIdx*NUM_PER_WI+1];
	datas[2] = sortData[gIdx*NUM_PER_WI+2];
	datas[3] = sortData[gIdx*NUM_PER_WI+3];

	keys[0] = (datas[0].m_key >> cb.m_startBit) & 0xff;
	keys[1] = (datas[1].m_key >> cb.m_startBit) & 0xff;
	keys[2] = (datas[2].m_key >> cb.m_startBit) & 0xff;
	keys[3] = (datas[3].m_key >> cb.m_startBit) & 0xff;

	int dst[NUM_PER_WI];
	for(int i=0; i<WG_SIZE; i++)
	{
		if( i==lIdx )
		{
			AtomInc1(ldsCurrentLocation[keys[0]], dst[0]);
			AtomInc1(ldsCurrentLocation[keys[1]], dst[1]);
			AtomInc1(ldsCurrentLocation[keys[2]], dst[2]);
			AtomInc1(ldsCurrentLocation[keys[3]], dst[3]);
		}
		GROUP_LDS_BARRIER;
	}
	sortDataOut[dst[0]] = datas[0];
	sortDataOut[dst[1]] = datas[1];
	sortDataOut[dst[2]] = datas[2];
	sortDataOut[dst[3]] = datas[3];
}

