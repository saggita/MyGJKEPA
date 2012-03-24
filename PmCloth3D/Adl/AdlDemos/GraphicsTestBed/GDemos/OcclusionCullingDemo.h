#pragma once

#include <Demo.h>
#include <AdlPrimitives/Scan/PrefixScan.h>
#include <AdlPrimitives/Sort/SortData.h>

#if defined(DX11RENDER)
class ShapeBase;



class OcclusionCullingDemo : public Demo
{
	public:
		COMPUTE_DX11;

		static Demo* createFunc( const Device* deviceData ) { return new OcclusionCullingDemo(deviceData); }

		OcclusionCullingDemo(const Device* deviceData);
		~OcclusionCullingDemo();

		void init();

		void render();

		void renderPre();

		void renderPost();

		void keyListener(unsigned char key);

		int getLowResWidth();
		int getLowResHeight();
		int getNCellsX();
		int getNCellsY();

	public:
		enum
		{
			MAX_BODIES = 2000, 
			LOW_RES_SCALE = 8,
			MAX_OVL_PER_OBJ = 10,

			CELL_RES = 16,

			MAX_CELL_X = 1920/(8*CELL_RES),
			MAX_CELL_Y = 1080/(8*CELL_RES),
		};

		struct CBuffer
		{
			XMMATRIX m_view;
			XMMATRIX m_projection;
			float m_maxDepth;
			int m_fullWidth;
			int m_lowResWidth;
			int m_lowResHeight;
			int m_nBodies;
			int m_maxOvlPerObj;
		};

		ShapeBase** m_shapes;
		float4* m_pos;
		u32 m_nVisibleIdx;
		u32* m_visibleFlag;

		DeviceRenderTargetDX11 m_colorRT;

		DeviceShaderDX11 m_depthPShader;
		DeviceShaderDX11 m_globalPixelShader;

		//	kernels
		Kernel m_zReduceKernel;
		Kernel m_queryKernel;
		Kernel m_countNOverlapCellsKernel;
		Kernel m_fillSortDataKernel;
		Kernel m_cullByTileKernel;

		//	kernels for debug
		Kernel m_kernel;
		Kernel m_upScaleKernel;

		Buffer<CBuffer> m_cBuffer;
		Buffer<float4> m_buffer;
		Buffer<u32> m_lowResDBuffer;
		Buffer<float4> m_bbsBuffer;
		Buffer<u32> m_visibleFlagBuffer;
		Buffer<u32> m_nOverlappedCellBuffer;
		Buffer<u32> m_offsetBuffer;
		Buffer<SortData> m_sortDataBuffer;
		Buffer<u32> m_lowerBoundBuffer;
		Buffer<u32> m_upperBoundBuffer;

		PrefixScan<(DeviceType)MyDeviceType>::Data* m_scanData;

		float m_maxDepth;


		int m_dispFlg0;
};



#else
class OcclusionCullingDemo : public Demo
{
	public:
		static Demo* createFunc( const Device* deviceData ) { return new OcclusionCullingDemo(deviceData); }

		OcclusionCullingDemo(const Device* deviceData){}
		~OcclusionCullingDemo(){}
};
#endif
