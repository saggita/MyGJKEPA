#define SHADER_PATH "..\\..\\AdlGraphics\\Shaders\\"
#define GET_SHADER_FULL_PATH(name, fullPathOut) sprintf(fullPathOut,"%s%s", SHADER_PATH, name)

#include <DXGI.h>
#include <Windowsx.h>
#include <D3D11SDKLayers.h>

#pragma comment(lib,"d3dcompiler.lib")


DeviceDX11* g_deviceData;


DeviceRenderTargetDX11 g_renderTarget;
DeviceRenderTargetDX11 g_depthStencil;

DeviceShaderDX11	g_defaultVertexShader;
DeviceShaderDX11	g_defaultPixelShader;
DeviceShaderDX11	g_quadVertexShader;
DeviceShaderDX11	g_quadPixelShader;
DeviceShaderDX11	g_bufferToRTPixelShader;
DeviceShaderDX11	g_textureMapWColorPixelShader;

DeviceShaderDX11	g_pointSpriteVertexShader;
DeviceShaderDX11	g_pointSpriteGeometryShader;
DeviceShaderDX11	g_pointSpritePixelShader;


ID3D11RasterizerState* g_rasterizerStateSolid = NULL;
ID3D11RasterizerState* g_rasterizerStateWireframe = NULL;

ID3D11DepthStencilState* g_depthTestOn = NULL;
ID3D11DepthStencilState* g_depthTestOff = NULL;

ID3D11SamplerState* g_defaultSampler;
ID3D11BlendState* g_blendState;

BufferDX11<ConstantBuffer>		g_constBuffer;

XMMATRIX g_World;
XMMATRIX g_View;
XMMATRIX g_Projection;
XMMATRIX g_ViewTr;
XMMATRIX g_ProjectionTr;

Array<RenderObject> g_debugRenderObj;
Array<RenderObject> g_renderObj;

AppendVertexBuffer* g_appVertexBuffer;
AppendIndexBuffer* g_appIndexBuffer;
AppendVertexBuffer* g_appDebugVertexBuffer;
AppendIndexBuffer* g_appDebugIndexBuffer;


#include <AdlGraphics/PostEffectDX11.h>
#include <AdlGraphics/FontDX11.h>



class AdlGraphics
{
	public:
		AdlGraphics();

		void allocate(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow, int wWidth, int wHeight);

		void renderLoop();

		void deallocate();

		static
		LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );


	private:
		HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow, int wWidth, int wHeight );
		void prepareGlobalResources();
		HRESULT InitDevice();

		static
		void OnMouse( UINT message, UINT shift );

		static
		void OnKeyboard( UINT nChar );

		static
		void OnResize();

		void render();
		void renderText();


	public:
		HINSTANCE g_hInst;
		HWND g_hWnd;

		static u32 g_mouseFlags;
		PostEffectSSAO* g_postEffect;

		FontDX11 g_font;


};

AdlGraphics adlGraphics;

