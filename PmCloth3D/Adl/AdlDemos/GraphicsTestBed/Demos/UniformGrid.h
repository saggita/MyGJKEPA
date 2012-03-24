#pragma once

#include <Adl/Adl.h>
#include <AdlPrimitives/Math/Math.h>

class UniformGridBase
{
	public:
		struct Config
		{
			float4 m_max;
			float4 m_min;
			float m_spacing;
		};

};


template<DeviceType TYPE>
class UniformGrid : public UniformGridBase
{
	public:
		typedef Launcher::BufferInfo BufferInfo;
		struct ConstData
		{
			float4 m_max;
			float4 m_min;
			int4 m_nCells;
			float m_gridScale;
			int m_n;
		};

		struct Data
		{
			float4 m_max;
			float4 m_min;
			int4 m_nCells;
			float m_gridScale;

			const Device* m_device;

			Kernel* m_clearKernel;
			Kernel* m_gridConstructionKernel;

			Buffer<u32>* m_counter;
			Buffer<u32>* m_gridData;
			Buffer<ConstData>* m_constBuffer;

			int getNCells() const { return m_nCells.x*m_nCells.y*m_nCells.z; }
		};

		static
		Data* allocate( const Device* device, const Config& cfg, bool allocateZeroCopyBuffer = false );

		static
		void deallocate(Data* data);

		static
		void execute( Data* data, Buffer<float4>& pos, int n );
};

#include <Demos/UniformGridDefines.h>
#include <Demos/UniformGrid.inl>
#include <Demos/UniformGridHost.inl>
