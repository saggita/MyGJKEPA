#pragma once

#include <Demo.h>
#include <AdlPrimitives/Sort/RadixSort.h>
#include <AdlPrimitives/Fill/Fill.h>


class AdlPrimitivesDemo : public Demo
{
	public:
		COMPUTE_DX11;
		static Demo* createFunc( const Device* device ) { return new AdlPrimitivesDemo( device ); }

		AdlPrimitivesDemo( const Device* device );
		~AdlPrimitivesDemo();

		void reset();

		void step(float dt);

		void render();

		void testSort( Buffer<SortData>& buf, int size, Stopwatch& sw );

		void testFill1( Buffer<int>& buf, int size, Stopwatch& sw );

		void testFill2( Buffer<int2>& buf, int size, Stopwatch& sw );

		void testFill4( Buffer<int4>& buf, int size, Stopwatch& sw );

		void test( Buffer<int2>& buf, int size, Stopwatch& sw );

	public:
		typedef RadixSort<(DeviceType)MyDeviceType> MySort;
		typedef Fill<(DeviceType)MyDeviceType> MyFill;
		typedef Launcher::BufferInfo BufferInfo;

};
