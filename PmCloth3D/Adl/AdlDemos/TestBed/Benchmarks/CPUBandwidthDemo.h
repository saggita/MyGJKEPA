#ifndef CPU_CACHE_DEMO_H
#define CPU_CACHE_DEMO_H

#include <Demo.h>


//	Run in release to get proper numbers
class CPUBandwidthDemo : public Demo
{
	public:
		static Demo* createFunc(const Device* deviceData) { return new CPUBandwidthDemo(); }

		CPUBandwidthDemo();
		~CPUBandwidthDemo();

		void step(float dt);

		void render();

		template<bool BANDWIDTH_TEST, int nTests>
		void testFunc();

		//	assuming cache line size is smaller than this
		struct Data4B
		{
			u32 m_data;
//			u32 m_padding[3];
		};

		enum
		{
			NUM_1KB = 1024/sizeof(Data4B), 
			NUM_64KB = NUM_1KB*64,
			NUM_128KB = NUM_1KB*128,
			NUM_256KB = NUM_1KB*256,
			NUM_512KB = NUM_1KB*512,
			NUM_1MB = NUM_1KB*1024,
			NUM_8MB = NUM_1MB*8,
			NUM_16MB = NUM_1MB*16,

			NUM_MAX = NUM_8MB,
		};
		Data4B* m_data;

		int m_sum;


};


#endif
