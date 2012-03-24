#pragma once

#include <Demo.h>
#include <Demos/AdlMapBuffer.h>
#include <Demos/UniformGrid.h>


class MixedParticlesDemo : public Demo
{
	public:
		COMPUTE_CL;

		static Demo* createFunc( const Device* deviceData ) { return new MixedParticlesDemo(deviceData); }

		MixedParticlesDemo( const Device* device );
		~MixedParticlesDemo();

		void reset();

		void step( float dt );

		void render();

		void renderPre();

		void renderPost();

		void resolveSrv( void** srv );

		enum
		{
			ENABLE_POST_EFFECT = 0,
			COPY_NATIVE_GPU = 1,

		};

	public:
		typedef Launcher::BufferInfo BufferInfo;

		typedef struct
		{
			float4 m_g;
			int m_numParticles;
			float m_dt;
			float m_scale;
			float m_e;

			int4 m_nCells;
			float4 m_spaceMin;
			float m_gridScale;

		}ConstBuffer;
		


		int m_nL;
		float4* m_pos;
		float4* m_vel;
		float4* m_force;
		struct Aabb* m_aabb;
		struct AabbUint* m_aabbUint;


		int m_nS;
		Buffer<float4> m_posSD;
		Buffer<float4> m_velSD;
		Buffer<float4> m_forceSD;
		Buffer<float4> m_forceIntD;
		Buffer<float4> m_posSNative;
		Buffer<float4> m_velSNative;

		MapBuffer m_posMapped;
		MapBuffer m_velMapped;
		MapBuffer m_forceMapped;
		MapBuffer m_gridMapped;
		MapBuffer m_gridCounterMapped;


		Kernel m_collideAllKernel;
		Kernel m_collideGridKernel;
		Kernel m_integrateKernel;
		Kernel m_copyPosVelKernel;

		Buffer<ConstBuffer> m_constBuffer;

		float4 m_planes[4];

		UniformGrid<TYPE_CL>::Data* m_ugData;

		//	host
		Device* m_deviceHost;
		UniformGrid<TYPE_HOST>::Data* m_hostUgData;


		//	rendering
		struct RConstBuffer
		{
			int m_width;
			int m_height;
		};

		DeviceDX11* m_deviceRender;

		DeviceRenderTargetDX11 m_colorRT;

		ID3D11RenderTargetView* m_rtv;
		ID3D11DepthStencilView* m_dsv;

		DeviceShaderDX11 m_gpVShader;
		DeviceShaderDX11 m_gpPShader;
		DeviceShaderDX11 m_gVShader;
		DeviceShaderDX11 m_gPShader;

		Kernel m_postProcessKernel;
		Buffer<float4> m_renderBuffer;
		Buffer<RConstBuffer> m_rConstBuffer;
};


