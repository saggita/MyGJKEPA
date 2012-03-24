#pragma once


#include <Adl/Adl.h>
#include <AdlPrimitives/Math/Math.h>
#include <AdlPrimitives/Scan/PrefixScan.h>
#include <AdlPrimitives/Sort/SortData.h>
#include <AdlPrimitives/Search/BoundSearch.h>
#include <AdlPrimitives/Sort/RadixSort.h>
#include <AdlPrimitives/Fill/Fill.h>

#include <xnamath.h>

class CSCulling
{
	public:
		struct Config
		{
			int m_maxLights;
			int m_nTilesPerLights;
			int m_tileRes;

			Config() : m_maxLights(1024), m_nTilesPerLights(128), m_tileRes(8){}
		};

		CSCulling( const Device* device, const Config& cfg );

		~CSCulling();

		void resize(int width, int height);

		void execute(const Buffer<float4>& spheres, int nSpheres, const XMMATRIX& view, const XMMATRIX& projection);

		void executePixelCulling(const Buffer<float>& depthBuffer, const Buffer<float4>& spheres, int nSpheres, 
			const XMMATRIX& view, const XMMATRIX& projection);


		Buffer<u32>* getLowerBoundBuffer() { return m_lowerBoundBuffer; }
		Buffer<u32>* getUpperBoundBuffer() { return m_upperBoundBuffer; }
		Buffer<SortData>* getIndexBuffer() { return m_indexBuffer; } // key: tileIdx, value: lightIdx
		Buffer<u32>* getPixelLightBitsBuffer() { return m_pixelLightBitsBuffer; }

		int getTileRes() { return m_cfg.m_tileRes; }

		int2 getNTiles();

	protected:
		void allocateBuffers(int width, int height);
		void deallocateBuffers();

	protected:
		typedef Launcher::BufferInfo BufferInfo;

		struct CBuffer
		{
			XMMATRIX m_view;
			XMMATRIX m_projection;
			XMMATRIX m_projectionInv;
			int m_width;
			int m_height;
			int m_nBodies;
			int m_maxOvlPerObj;
			int m_tileRes;
		};


		DeviceDX11 m_device;
		Config m_cfg;
		int2 m_size;


		Kernel m_countNOverlapCellsKernel;
		Kernel m_fillSortDataKernel;
		Kernel m_pixelLightCullingKernel;


		Buffer<u32>* m_lowerBoundBuffer;
		Buffer<u32>* m_upperBoundBuffer;
		Buffer<SortData>* m_indexBuffer;

		Buffer<u32>* m_visibleFlagBuffer;
		Buffer<u32>* m_nOverlappedCellBuffer;
		Buffer<u32>* m_offsetBuffer;
		Buffer<u32>* m_pixelLightBitsBuffer;
		Buffer<CBuffer>* m_constBuffer[4];

		PrefixScan<TYPE_DX11>::Data* m_scanData;
		BoundSearch<TYPE_DX11>::Data* m_searchData;
		RadixSort<TYPE_DX11>::Data* m_sortData;
		Fill<TYPE_DX11>::Data* m_fillData;
		
};




