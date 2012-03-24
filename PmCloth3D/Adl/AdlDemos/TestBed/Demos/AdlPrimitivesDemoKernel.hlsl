typedef uint u32;

#define GET_GROUP_IDX groupIdx.x
#define GET_LOCAL_IDX localIdx.x
#define GET_GLOBAL_IDX globalIdx.x
#define GROUP_LDS_BARRIER GroupMemoryBarrierWithGroupSync()
#define GROUP_MEM_FENCE
#define DEFAULT_ARGS uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID
#define AtomInc(x) InterlockedAdd(x, 1)
#define AtomInc1(x, out) InterlockedAdd(x, 1, out)

#define make_uint4 uint4
#define make_uint2 uint2
#define make_int2 int2

uint4 SELECT_UINT4(uint4 b,uint4 a,uint4 condition ){ return  make_uint4( ((condition).x)?a.x:b.x, ((condition).y)?a.y:b.y, ((condition).z)?a.z:b.z, ((condition).w)?a.w:b.w ); }


#define STORE4( address, f4 ) Store4( address, f4 )

#define WG_SIZE 128
#define NUM_PER_WI 4

#define GET_GROUP_SIZE WG_SIZE

#define BITS_PER_PASS 4


cbuffer CB : register( b0 )
{
	int m_n;
	int m_startBit;
	int m_numGroups;
	int m_switch;
};

typedef struct
{
	u32 m_key; 
	u32 m_value;
}SortData;


groupshared u32 ldsSortData[ WG_SIZE*NUM_PER_WI + 16 ];
groupshared u32 sorterSharedMemory[WG_SIZE*NUM_PER_WI*2];
groupshared u32 ldsHistogram[ 16 ];

uint4 prefixScanVector( uint4 data )
{
	data.y += data.x;
	data.w += data.z;
	data.z += data.y;
	data.w += data.y;
	return data;
}

uint prefixScanVectorEx( inout uint4 data )
{
//	uint4 backup = data;
//	data.y += data.x;
//	data.w += data.z;
//	data.z += data.y;
//	data.w += data.y;
//	uint sum = data.w;
//	data -= backup;
//	return sum;
	uint4 d = data;
	data.x = 0;
	data.y = d.x;
	data.z = d.y + data.y;
	data.w = d.z + data.z;
	return data.w + d.w;

}

uint4 localPrefixSum128V( uint4 pData, uint lIdx, inout uint totalSum )
{
	{	//	Set data
		ldsSortData[lIdx] = 0;
		ldsSortData[lIdx+WG_SIZE] = prefixScanVectorEx( pData );
	}

	GROUP_LDS_BARRIER;

	{	//	Prefix sum
		int idx = 2*lIdx + (WG_SIZE+1);
		if( lIdx < 64 )
		{
			ldsSortData[idx] += ldsSortData[idx-1];
			GROUP_MEM_FENCE;
			ldsSortData[idx] += ldsSortData[idx-2];					
			GROUP_MEM_FENCE;
			ldsSortData[idx] += ldsSortData[idx-4];
			GROUP_MEM_FENCE;
			ldsSortData[idx] += ldsSortData[idx-8];
			GROUP_MEM_FENCE;
			ldsSortData[idx] += ldsSortData[idx-16];
			GROUP_MEM_FENCE;
			ldsSortData[idx] += ldsSortData[idx-32];		
			GROUP_MEM_FENCE;
			ldsSortData[idx] += ldsSortData[idx-64];
			GROUP_MEM_FENCE;

			ldsSortData[idx-1] += ldsSortData[idx-2];
			GROUP_MEM_FENCE;
		}
	}

	GROUP_LDS_BARRIER;

	totalSum = ldsSortData[WG_SIZE*2-1];
	uint addValue = ldsSortData[lIdx+127];
	return pData + make_uint4(addValue, addValue, addValue, addValue);
}


void localPrefixScan128Dual( uint pData0, uint pData1, uint lIdx, 
							inout uint rank0, inout uint rank1,
							inout uint totalSum0, inout uint totalSum1 )
{
	{	//	Set data
		ldsSortData[lIdx] = 0;
		ldsSortData[lIdx+WG_SIZE] = pData0;
		ldsSortData[2*WG_SIZE+lIdx] = 0;
		ldsSortData[2*WG_SIZE+lIdx+WG_SIZE] = pData1;
	}

	GROUP_LDS_BARRIER;

//	if( lIdx < 128 ) // todo. assert wg size is 128
	{	//	Prefix sum
		int blockIdx = lIdx/64;
		int groupIdx = lIdx%64;
		int idx = 2*groupIdx + (WG_SIZE+1) + (2*WG_SIZE)*blockIdx;

		ldsSortData[idx] += ldsSortData[idx-1];
		ldsSortData[idx] += ldsSortData[idx-2];
		ldsSortData[idx] += ldsSortData[idx-4];
		ldsSortData[idx] += ldsSortData[idx-8];
		ldsSortData[idx] += ldsSortData[idx-16];
		ldsSortData[idx] += ldsSortData[idx-32];		
		ldsSortData[idx] += ldsSortData[idx-64];

		ldsSortData[idx-1] += ldsSortData[idx-2];
	}

	GROUP_LDS_BARRIER;

	totalSum0 = ldsSortData[WG_SIZE*2-1];
	rank0 = ldsSortData[lIdx+127];
	totalSum1 = ldsSortData[2*WG_SIZE+WG_SIZE*2-1];
	rank1 = ldsSortData[2*WG_SIZE+lIdx+127];
}


void generateHistogram(u32 lIdx, u32 wgIdx, 
		uint4 sortedData)
{
    if( lIdx < (1<<BITS_PER_PASS) )
    {
    	ldsSortData[lIdx] = 0;
    }

	int mask = ((1<<BITS_PER_PASS)-1);
	uint4 keys = make_uint4( (sortedData.x)&mask, (sortedData.y)&mask, (sortedData.z)&mask, (sortedData.w)&mask );

	GROUP_LDS_BARRIER;
	
	AtomInc( ldsSortData[keys.x] );
	AtomInc( ldsSortData[keys.y] );
	AtomInc( ldsSortData[keys.z] );
	AtomInc( ldsSortData[keys.w] );
}




void localPrefixSum128Dual( uint4 pData0, uint4 pData1, uint lIdx, 
						   inout uint4 dataOut0, inout uint4 dataOut1, 
						   inout uint totalSum0, inout uint totalSum1 )
{
/*
	dataOut0 = localPrefixSum128V( pData0, lIdx, totalSum0 );
	GROUP_LDS_BARRIER;
	dataOut1 = localPrefixSum128V( pData1, lIdx, totalSum1 );
	return;
*/

	uint4 backup0 = pData0;
	uint4 backup1 = pData1;

	{	// Prefix sum in a vector
		pData0 = prefixScanVector( pData0 );
		pData1 = prefixScanVector( pData1 );
	}

	{	//	Set data
		sorterSharedMemory[lIdx] = 0;
		sorterSharedMemory[lIdx+WG_SIZE] = pData0.w;
		sorterSharedMemory[2*WG_SIZE+lIdx] = 0;
		sorterSharedMemory[2*WG_SIZE+lIdx+WG_SIZE] = pData1.w;
	}

	GROUP_LDS_BARRIER;

//	if( lIdx < 128 ) // todo. assert wg size is 128
	{	//	Prefix sum
		int blockIdx = lIdx/64;
		int groupIdx = lIdx%64;
		int idx = 2*groupIdx + (WG_SIZE+1) + (2*WG_SIZE)*blockIdx;

		sorterSharedMemory[idx] += sorterSharedMemory[idx-1];
		sorterSharedMemory[idx] += sorterSharedMemory[idx-2];
		sorterSharedMemory[idx] += sorterSharedMemory[idx-4];
		sorterSharedMemory[idx] += sorterSharedMemory[idx-8];
		sorterSharedMemory[idx] += sorterSharedMemory[idx-16];
		sorterSharedMemory[idx] += sorterSharedMemory[idx-32];		
		sorterSharedMemory[idx] += sorterSharedMemory[idx-64];

		sorterSharedMemory[idx-1] += sorterSharedMemory[idx-2];
	}

	GROUP_LDS_BARRIER;

	totalSum0 = sorterSharedMemory[WG_SIZE*2-1];
	{
		uint addValue = sorterSharedMemory[lIdx+127];
		dataOut0 = pData0 + uint4(addValue, addValue, addValue, addValue) - backup0;
	}

	totalSum1 = sorterSharedMemory[2*WG_SIZE+WG_SIZE*2-1];
	{
		uint addValue = sorterSharedMemory[2*WG_SIZE+lIdx+127];
		dataOut1 = pData1 + uint4(addValue, addValue, addValue, addValue) - backup1;
	}
}

uint4 extractKeys(uint4 data, uint targetKey)
{
	uint4 key;
//	key.x = data.x == targetKey ? 1:0;
//	key.y = data.y == targetKey ? 1:0;
//	key.z = data.z == targetKey ? 1:0;
//	key.w = data.w == targetKey ? 1:0;

	key.x = (data.x == targetKey);
	key.y = (data.y == targetKey);
	key.z = (data.z == targetKey);
	key.w = (data.w == targetKey);

	return key;
}

//	assuming data is 0 or 1
uint compress(uint4 data)
{
	return data.x | data.y<<1 | data.z<<2 | data.w<<3;
}

uint4 uncompress(uint data)
{
	uint4 key;
	key.x = data & 1;
	key.y = (data>>1) & 1;
	key.z = (data>>2) & 1;
	key.w = (data>>3) & 1;
	return key;
}

uint4 expandLower4Bits( uint data )
{
}

uint4 extractKeysByBits(uint4 data, uint targetKey)
{
	uint4 key;
	uint mask = 1<<targetKey;
	key.x = (data.x & mask) >> targetKey;
	key.y = (data.y & mask) >> targetKey;
	key.z = (data.z & mask) >> targetKey;
	key.w = (data.w & mask) >> targetKey;
	return key;
}

uint packKeys(uint lower, uint upper)
{
	return lower|(upper<<8);
}

uint packKeys4(uint data0, uint data1, uint data2, uint data3)
{
	return data0|(data1<<8)|(data2<<16)|(data3<<24);
}

uint extract( uint data, int shift )
{
	return (data>>shift)&0xff;
}

uint extract0( uint data )
{
	return data&0xff;
}

uint extract8( uint data )
{
	return (data>>8)&0xff;
}

uint extract16( uint data )
{
	return (data>>16)&0xff;
}

uint extract24( uint data )
{
	return (data>>24)&0xff;
}

/*
uint4 extract0( uint4 data )
{
	return uint4( data.x&0xffff, data.y&0xffff, data.z&0xffff, data.w&0xffff );
}

uint4 extractUpper( uint4 data )
{
	return uint4( (data.x>>16)&0xffff, (data.y>>16)&0xffff, (data.z>>16)&0xffff, (data.w>>16)&0xffff );
}
*/

RWStructuredBuffer<SortData> gBuffer : register( u0 );
RWStructuredBuffer<u32> ldsHistogramOut0 : register( u1 );
RWStructuredBuffer<u32> ldsHistogramOut1 : register( u2 );


groupshared u32 localPrefixSum[1<<BITS_PER_PASS];

[numthreads(WG_SIZE, 1, 1)]
void FillInt4Kernel( DEFAULT_ARGS )
{
	int gIdx = GET_GLOBAL_IDX;
	u32 lIdx = GET_LOCAL_IDX;
	u32 wgIdx = GET_GROUP_IDX;
	u32 wgSize = GET_GROUP_SIZE;
	
    uint4 localAddr = make_uint4(lIdx*4+0,lIdx*4+1,lIdx*4+2,lIdx*4+3);
//	if( 4*gIdx >= m_n ) return;

	SortData sortData[4];

	{	//	load
		u32 offset = WG_SIZE*NUM_PER_WI*wgIdx + 4*lIdx;
		sortData[0] = gBuffer[ offset + 0 ];
		sortData[1] = gBuffer[ offset + 1 ];
		sortData[2] = gBuffer[ offset + 2 ];
		sortData[3] = gBuffer[ offset + 3 ];
	}

	if( m_switch > 1 )
	{
		if(0)
		{
			for(int bitIdx=m_startBit; bitIdx<(m_startBit+BITS_PER_PASS); bitIdx++)
			{
				uint4 dstAddr;
				{
					u32 mask = (1<<bitIdx);
					uint4 cmpResult = make_uint4( sortData[0].m_key & mask, sortData[1].m_key & mask, sortData[2].m_key & mask, sortData[3].m_key & mask );
					uint4 prefixSum = SELECT_UINT4( make_uint4(1,1,1,1), make_uint4(0,0,0,0), cmpResult != make_uint4(0,0,0,0) );
					u32 total;
					prefixSum = localPrefixSum128V( prefixSum, lIdx, total );
					dstAddr = localAddr - prefixSum + make_uint4( total, total, total, total );
					dstAddr = SELECT_UINT4( prefixSum, dstAddr, cmpResult != make_uint4(0, 0, 0, 0) );
				}

				{
					GROUP_LDS_BARRIER;

					ldsSortData[dstAddr.x] = sortData[0].m_key;
					ldsSortData[dstAddr.y] = sortData[1].m_key;
					ldsSortData[dstAddr.z] = sortData[2].m_key;
					ldsSortData[dstAddr.w] = sortData[3].m_key;

					GROUP_LDS_BARRIER;

					sortData[0].m_key = ldsSortData[localAddr.x];
					sortData[1].m_key = ldsSortData[localAddr.y];
					sortData[2].m_key = ldsSortData[localAddr.z];
					sortData[3].m_key = ldsSortData[localAddr.w];

					GROUP_LDS_BARRIER;

					ldsSortData[dstAddr.x] = sortData[0].m_value;
					ldsSortData[dstAddr.y] = sortData[1].m_value;
					ldsSortData[dstAddr.z] = sortData[2].m_value;
					ldsSortData[dstAddr.w] = sortData[3].m_value;

					GROUP_LDS_BARRIER;

					sortData[0].m_value = ldsSortData[localAddr.x];
					sortData[1].m_value = ldsSortData[localAddr.y];
					sortData[2].m_value = ldsSortData[localAddr.z];
					sortData[3].m_value = ldsSortData[localAddr.w];

					GROUP_LDS_BARRIER;
				}
			}
		}
		else
		{
			if( lIdx < (1<<BITS_PER_PASS) ) localPrefixSum[lIdx] = 0;

			uint4 newOffsets = uint4(0,0,0,0);
			int localOffset = 0;
			uint4 b = uint4((sortData[0].m_key>>m_startBit) & 0xf, (sortData[1].m_key>>m_startBit) & 0xf, (sortData[2].m_key>>m_startBit) & 0xf, (sortData[3].m_key>>m_startBit) & 0xf);

			//	so iterating twice. For each bucket but not for each bit
			for(int targetKey=0; targetKey<(1<<BITS_PER_PASS); targetKey+=8)
//			int targetKey = 0;
			{
				uint laneKeySumPacked[2];
				{	//	1. pack 4
					uint s[8];
					for(int ie=0; ie<8; ie++)
					{
						uint4 scannedKey;
						scannedKey = extractKeys( b, targetKey+ie );
						s[ie] = prefixScanVectorEx( scannedKey );
					}
					laneKeySumPacked[0] = packKeys4( s[0], s[1], s[2], s[3] );
					laneKeySumPacked[1] = packKeys4( s[4], s[5], s[6], s[7] );
				}

				uint dstAddressPacked[2];
				{	//	2. scan
					uint totalSumPacked[2];

					localPrefixScan128Dual( laneKeySumPacked[0], laneKeySumPacked[1], lIdx, dstAddressPacked[0], dstAddressPacked[1], totalSumPacked[0], totalSumPacked[1] );

					GROUP_LDS_BARRIER;
					if( lIdx < 8 )	//	scan counts for 8 buckets
					{
						int hIdx = 8+lIdx;
						ldsHistogram[lIdx] = 0;
						ldsHistogram[hIdx] = extract( totalSumPacked[lIdx>>2], 8*(lIdx&3) );
						GROUP_MEM_FENCE;

						ldsHistogram[hIdx] += ldsHistogram[hIdx-1];
						GROUP_MEM_FENCE;
						ldsHistogram[hIdx] += ldsHistogram[hIdx-2];
						GROUP_MEM_FENCE;
						ldsHistogram[hIdx] += ldsHistogram[hIdx-4];
						GROUP_MEM_FENCE;

						localPrefixSum[targetKey+lIdx] = localOffset + ldsHistogram[hIdx-1];
					}

					localOffset += ldsHistogram[15];
				}
			
				GROUP_LDS_BARRIER;

				{//	3. calculate offset using these results
					{	int ie = b.x%8;
						uint4 key = extractKeys( b, targetKey+ie );
						uint4 scannedKey = key;
						prefixScanVectorEx( scannedKey );

						uint offset = localPrefixSum[targetKey + ie] + extract( dstAddressPacked[ie>>2], 8*(ie&3) );
						uint4 dstAddress = uint4( offset, offset, offset, offset ) + scannedKey;

						newOffsets.x += (key.x)? dstAddress.x: 0;
					}
					{	int ie = b.y%8;
						uint4 key = extractKeys( b, targetKey+ie );
						uint4 scannedKey = key;
						prefixScanVectorEx( scannedKey );

						uint offset = localPrefixSum[targetKey + ie] + extract( dstAddressPacked[ie>>2], 8*(ie&3) );
						uint4 dstAddress = uint4( offset, offset, offset, offset ) + scannedKey;

						newOffsets.y += (key.y)? dstAddress.y: 0;
					}
					{	int ie = b.z%8;
						uint4 key = extractKeys( b, targetKey+ie );
						uint4 scannedKey = key;
						prefixScanVectorEx( scannedKey );

						uint offset = localPrefixSum[targetKey + ie] + extract( dstAddressPacked[ie>>2], 8*(ie&3) );
						uint4 dstAddress = uint4( offset, offset, offset, offset ) + scannedKey;

						newOffsets.z += (key.z)? dstAddress.z: 0;
					}
					{	int ie = b.w%8;
						uint4 key = extractKeys( b, targetKey+ie );
						uint4 scannedKey = key;
						prefixScanVectorEx( scannedKey );

						uint offset = localPrefixSum[targetKey + ie] + extract( dstAddressPacked[ie>>2], 8*(ie&3) );
						uint4 dstAddress = uint4( offset, offset, offset, offset ) + scannedKey;

						newOffsets.w += (key.w)? dstAddress.w: 0;
					}
				}
			}


			//===
			{
				GROUP_LDS_BARRIER;

				ldsSortData[newOffsets.x] = sortData[0].m_key;
				ldsSortData[newOffsets.y] = sortData[1].m_key;
				ldsSortData[newOffsets.z] = sortData[2].m_key;
				ldsSortData[newOffsets.w] = sortData[3].m_key;

				GROUP_LDS_BARRIER;

				sortData[0].m_key = ldsSortData[localAddr.x];
				sortData[1].m_key = ldsSortData[localAddr.y];
				sortData[2].m_key = ldsSortData[localAddr.z];
				sortData[3].m_key = ldsSortData[localAddr.w];

				GROUP_LDS_BARRIER;

				ldsSortData[newOffsets.x] = sortData[0].m_value;
				ldsSortData[newOffsets.y] = sortData[1].m_value;
				ldsSortData[newOffsets.z] = sortData[2].m_value;
				ldsSortData[newOffsets.w] = sortData[3].m_value;

				GROUP_LDS_BARRIER;

				sortData[0].m_value = ldsSortData[localAddr.x];
				sortData[1].m_value = ldsSortData[localAddr.y];
				sortData[2].m_value = ldsSortData[localAddr.z];
				sortData[3].m_value = ldsSortData[localAddr.w];
			}
		}
	}
/*
	if( m_switch > 0 )
	{
		uint4 localKeys = make_uint4( sortData[0].m_key>>m_startBit, sortData[1].m_key>>m_startBit, 
			sortData[2].m_key>>m_startBit, sortData[3].m_key>>m_startBit );

		generateHistogram( lIdx, wgIdx, localKeys );

		GROUP_LDS_BARRIER;

		int nBins = (1<<BITS_PER_PASS);
		if( lIdx < nBins )
		{
     		u32 histValues = ldsSortData[lIdx];

     		u32 globalAddresses = nBins*wgIdx + lIdx;
     		u32 globalAddressesRadixMajor = m_numGroups*lIdx + wgIdx;
		
//     		ldsHistogramOut0[globalAddressesRadixMajor] = histValues;
//     		ldsHistogramOut1[globalAddresses] = histValues;
			sortData[0].m_key += histValues;
		}
	}
*/
//	if( m_switch < -1 )
	{	//	store
		u32 offset = WG_SIZE*NUM_PER_WI*wgIdx + 4*lIdx;
		gBuffer[ offset + 0 ] = sortData[0];
		gBuffer[ offset + 1 ] = sortData[1];
		gBuffer[ offset + 2 ] = sortData[2];
		gBuffer[ offset + 3 ] = sortData[3];
	}
}


