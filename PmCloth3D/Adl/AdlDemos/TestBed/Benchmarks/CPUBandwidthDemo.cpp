#include <Benchmarks/CPUBandwidthDemo.h>


CPUBandwidthDemo::CPUBandwidthDemo()
: Demo()
{
	m_data = new Data4B[NUM_MAX];
	m_sum = 0;

	step(0.f);
}

CPUBandwidthDemo::~CPUBandwidthDemo()
{
	delete [] m_data;
}

#define UNROLL8(x) x;x;x;x; x;x;x;x;
#define INCREMENT8(x) {int iii=idx+0;x;}{int iii=idx+1;x;}{int iii=idx+2;x;}{int iii=idx+3;x;} \
	{int iii=idx+4;x;}{int iii=idx+5;x;}{int iii=idx+6;x;}{int iii=idx+7;x;}

template<bool BANDWIDTH_TEST, int nTests>
void CPUBandwidthDemo::testFunc()
{
	if( BANDWIDTH_TEST )
	{
		printf("== Bandwidth Test (%d)==\n", sizeof(Data4B));
	}
	else
	{
		printf("== Latency Test ==\n");
	}

	Stopwatch sw;

	for(int i=NUM_1KB*4; i<=NUM_MAX; i<<=1)
	{
		m_data[0].m_data = 1;
		for(int ii=1; ii<i; ii++)
		{
			while(1)
			{
				m_data[ii].m_data = ii+129;
				m_data[ii].m_data = m_data[ii].m_data%i;
				if( m_data[ii].m_data != ii ) break;
			}
		}

		sw.start();
		u32 s=0;
		int addr=0;
		for(int ii=0; ii<nTests; ii++)
		{
			for(int j=0; j<i; j+=8*4*4) 
			{
				if(BANDWIDTH_TEST)
				{	
					{int idx = j+8*0; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*1; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*2; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*3; INCREMENT8( s += m_data[iii].m_data );}

					{int idx = j+8*4; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*5; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*6; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*7; INCREMENT8( s += m_data[iii].m_data );}

					{int idx = j+8*8; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*9; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*10; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*11; INCREMENT8( s += m_data[iii].m_data );}

					{int idx = j+8*12; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*13; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*14; INCREMENT8( s += m_data[iii].m_data );}
					{int idx = j+8*15; INCREMENT8( s += m_data[iii].m_data );}
				}
				else
				{
					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );

					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );

					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );

					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );
					UNROLL8( addr = m_data[addr].m_data );
					s += addr;
				}
			}
		}
		m_sum += s;

		sw.stop();

		float tInS = sw.getMs()/(float)nTests/1000.f;
		int sizeInKb = i/NUM_1KB;
		int sizeInMb = i/NUM_1KB/1024;
		float tInNs = tInS*1e9f;
		float latencyInNs = tInNs/i;

		if( BANDWIDTH_TEST )
		{
			printf("%dKB	%3.2fGB/s\n", sizeInKb, (float)sizeInKb/tInS/1024.f/1024.f);
		}
		else
		{
			printf("%dKB	%3.2fns\n", sizeInKb, latencyInNs);
		}
	}
}


void CPUBandwidthDemo::step(float dt)
{
	Stopwatch sw;


	const int nTests = 100;
	const bool BANDWIDTH_TEST = 0;

	testFunc<true, 50>();
	testFunc<false, 50>();

	printf("\n");
}

void CPUBandwidthDemo::render()
{


}

