__inline
IDXGIAdapter* getAdapterFromDevice( const DeviceDX11* deviceData )
{
	IDXGIDevice* pDXGIDevice;
	IDXGIAdapter * pDXGIAdapter;

	ADLASSERT( deviceData->m_device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice) == S_OK );
	ADLASSERT( pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter) == S_OK );

	pDXGIDevice->Release();
//	pDXGIAdapter->Release();
	return pDXGIAdapter;
}

__inline
HRESULT AdlGraphics::InitWindow( HINSTANCE hInstance, int nCmdShow, int wWidth, int wHeight )
{
	g_wWidth = wWidth;
	g_wHeight = wHeight;

	// Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = 0;//LoadIcon( hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = 0;//LoadIcon( wcex.hInstance, ( LPCTSTR )IDI_TUTORIAL1 );
    if( !RegisterClassEx( &wcex ) ) return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, g_wWidth, g_wHeight };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    g_hWnd = CreateWindow( L"TutorialWindowClass", L"SmallDX11Engine", WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );

    if( !g_hWnd ) return E_FAIL;

//    ShowWindow( g_hWnd, nCmdShow );

    return S_OK;
}

__inline
void setWindow()
{
	{	//	resize swap chain & render target
		HRESULT hr;
		DeviceRenderTargetDX11::deleteRenderTarget( g_deviceData, g_renderTarget );
		DeviceRenderTargetDX11::deleteRenderTarget( g_deviceData, g_depthStencil );

		g_deviceData->m_swapChain->ResizeBuffers( 1, g_wWidth, g_wHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0 );

		{
			ID3D11Texture2D* pBackBuffer = NULL;
			hr = g_deviceData->m_swapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
			ADLASSERT( hr == S_OK );
        
			hr = g_deviceData->m_device->CreateRenderTargetView( pBackBuffer, NULL, &g_renderTarget.m_renderTarget );
			pBackBuffer->Release();
			ADLASSERT( hr == S_OK );
		}
		DeviceRenderTargetDX11::createRenderTarget( g_deviceData, g_wWidth, g_wHeight, g_depthStencil, DeviceRenderTargetDX11::TYPE_DEPTH_STENCIL );
	}
	{	//	Viewport
		D3D11_VIEWPORT vp;
		vp.Width = (FLOAT)g_wWidth;
		vp.Height = (FLOAT)g_wHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		g_deviceData->m_context->RSSetViewports( 1, &vp );
	}
	{	//	matrices
		g_World = XMMatrixIdentity();
/*
		XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, -4.0f, 0.0f );
		XMVECTOR At = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
		XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
		g_View = XMMatrixLookAtLH( Eye, At, Up );

		g_Projection = XMMatrixPerspectiveFovLH( XM_PI*g_fov/180.f, g_wWidth / (FLOAT)g_wHeight, 0.01f, 100.0f );
void getMatrices( const float4& eye, const float4& at, const float4& up, 
	float fov, float aspectRatio, float near, float far,
	void* viewMatrix, void* projectionMatrix)
*/
		getMatrices<false>( make_float4(0,0,-4,0), make_float4(0,0,0,0), make_float4(0,1,0,0), 
			XM_PI*g_fov/180.f, g_wWidth / (FLOAT)g_wHeight, 0.01f, 100.0f, &g_View, &g_Projection );
	}
}

__inline
HRESULT AdlGraphics::InitDevice()
{
	int deviceIdx = 0;//DeviceUtils::getNumDevices()-1;
	deviceIdx = 0;
	{
		DeviceUtils::Config cfg;
		g_deviceData = (DeviceDX11*)DeviceUtils::allocate( TYPE_DX11, cfg );
	}
//	DeviceUtilsDX11::initDevice( g_deviceData, DeviceUtilsDX11::DRIVER_HARDWARE, deviceIdx );

    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

	{	//	create swap chain
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory( &sd, sizeof( sd ) );
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = g_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		IDXGIFactory* factory;
		IDXGIAdapter* adapter = getAdapterFromDevice( g_deviceData );

		ADLASSERT( adapter->GetParent(__uuidof(IDXGIFactory), (void **)&factory) == S_OK );
		ADLASSERT( factory->CreateSwapChain( g_deviceData->m_device, &sd , &g_deviceData->m_swapChain ) == S_OK );

		factory->MakeWindowAssociation( g_hWnd, DXGI_MWA_VALID );

		factory->Release();
		adapter->Release();
	}
	ADLASSERT( hr == S_OK );

	setWindow();

	return S_OK;
}

void AdlGraphics::prepareGlobalResources()
{
	g_mouseFlags = false;

	HRESULT hr = S_OK;

	{	//	shader creation
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		{	//	shader
			char path[128];
			GET_SHADER_FULL_PATH( "FixedShader.fx", path );
			ShaderUtilsDX11 builder( g_deviceData, path );
			builder.createVertexShader( "VS", g_defaultVertexShader, ARRAYSIZE( layout ), layout );
			builder.createPixelShader( "PS", g_defaultPixelShader );
			builder.createVertexShader( "QuadVS", g_quadVertexShader, ARRAYSIZE( layout ), layout );
			builder.createPixelShader( "QuadPS", g_quadPixelShader );
			builder.createPixelShader( "BufferToRTPS", g_bufferToRTPixelShader );
			builder.createPixelShader( "TextureMapWColorPS", g_textureMapWColorPixelShader );
		}

		{
			char path[128];
			GET_SHADER_FULL_PATH( "PointSpriteShader.hlsl", path );
			ShaderUtilsDX11 builder( g_deviceData, path );
			builder.createVertexShader( "VS", g_pointSpriteVertexShader, ARRAYSIZE( layout ), layout );
			builder.createGeometryShader( "ParticleGS", g_pointSpriteGeometryShader );
			builder.createPixelShader( "PS", g_pointSpritePixelShader );
		}

		// Create the constant buffer
//		DeviceUtilsDX11::createDeviceBuffer<ConstantBuffer>( g_deviceData, 1, g_constBuffer, DeviceBufferBase::BUFFER_CONST );
		g_deviceData->allocate<ConstantBuffer>( &g_constBuffer, 1, BufferBase::BUFFER_CONST );

		g_appVertexBuffer = new AppendVertexBuffer( g_deviceData );
		g_appIndexBuffer = new AppendIndexBuffer( g_deviceData );

		g_appDebugVertexBuffer = new AppendVertexBuffer( g_deviceData );
		g_appDebugIndexBuffer = new AppendIndexBuffer( g_deviceData );
	}

	{	//	wireframe, solid rasterizer state
		D3D11_RASTERIZER_DESC RasterDesc;
		{
			ZeroMemory( &RasterDesc, sizeof( D3D11_RASTERIZER_DESC ) );
			RasterDesc.FillMode = D3D11_FILL_SOLID;
			RasterDesc.CullMode = D3D11_CULL_BACK;
			RasterDesc.DepthClipEnable = TRUE;
			g_deviceData->m_device->CreateRasterizerState( &RasterDesc, &g_rasterizerStateSolid );
		}
		{
			RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
			g_deviceData->m_device->CreateRasterizerState( &RasterDesc, &g_rasterizerStateWireframe );
		}

		D3D11_SAMPLER_DESC sampDesc;
		ZeroMemory( &sampDesc, sizeof(sampDesc) );
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		g_deviceData->m_device->CreateSamplerState( &sampDesc, &g_defaultSampler );
	}

	{	//	depth test
		D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
		ZeroMemory( &DepthStencilDesc, sizeof(DepthStencilDesc) );
		DepthStencilDesc.DepthEnable = TRUE;
		DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		DepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
		g_deviceData->m_device->CreateDepthStencilState( &DepthStencilDesc, &g_depthTestOn );

		DepthStencilDesc.DepthEnable = FALSE;
		DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		g_deviceData->m_device->CreateDepthStencilState( &DepthStencilDesc, &g_depthTestOff );
	}

	{
		D3D11_BLEND_DESC BlendStateDesc;
		ZeroMemory( &BlendStateDesc, sizeof(BlendStateDesc) );
		BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
		BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
		g_deviceData->m_device->CreateBlendState( &BlendStateDesc, &g_blendState );
	}


	g_postEffect = new PostEffectSSAO( g_deviceData );
	g_postEffect->resize( g_wWidth, g_wHeight );

	char path[128];
	GET_SHADER_FULL_PATH( "Font.dds", path );
	g_font.allocate( g_deviceData, path );
}



__inline
void writeToBmp(char* filename)
{
	ID3D11Texture2D* backBuffer;
	HRESULT hr = g_deviceData->m_swapChain->GetBuffer(0, __uuidof(backBuffer),
		reinterpret_cast<void**>(&backBuffer));

	D3D11_TEXTURE2D_DESC texDesc;
	backBuffer->GetDesc( &texDesc );

	int w = texDesc.Width;
	int h = texDesc.Height;

	ID3D11Texture2D* sTexture = 0;
	D3D11_TEXTURE2D_DESC texdesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, w, h,
		1, 1, 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ);
	hr = g_deviceData->m_device->CreateTexture2D(&texdesc, NULL, &sTexture);

	D3D11_MAPPED_SUBRESOURCE MappedVelResource = {0};

	D3D11_BOX destRegion = CD3D11_BOX(0,0,0,w*sizeof(u8)*4, h*sizeof(u8)*4, 1);

	g_deviceData->m_context->CopySubresourceRegion( sTexture, 0, 0, 0, 0 , backBuffer, 0, &destRegion	);

	hr = g_deviceData->m_context->Map(sTexture, 0, D3D11_MAP_READ, 0, &MappedVelResource);
	u8* ptr = (u8*)MappedVelResource.pData;

	BmpUtils::Image img(w,h);

	for(int i=0; i<w; i++) for(int j=0; j<h; j++)
	{
		int idx = i+j*w;
		img.data[idx].r = (u32)(ptr[idx*4+0]);
		img.data[idx].g = (u32)(ptr[idx*4+1]);
		img.data[idx].b = (u32)(ptr[idx*4+2]);
	}

	BmpUtils::write( filename, &img );

	g_deviceData->m_context->Unmap( sTexture, 0 );
	sTexture->Release();

	backBuffer->Release();
}

void AdlGraphics::OnResize()
{
	setWindow();

	if( adlGraphics.g_postEffect )
		adlGraphics.g_postEffect->resize( g_wWidth, g_wHeight );

    return;
}

//===
//===

AdlGraphics::AdlGraphics()
{
	g_postEffect = 0;
}



void AdlGraphics::allocate(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow, int wWidth, int wHeight)
{
	UNREFERENCED_PARAMETER( hPrevInstance );
	UNREFERENCED_PARAMETER( lpCmdLine );

	if( FAILED( InitWindow( hInstance, nCmdShow, INIT_WIDTH, INIT_HEIGHT ) ) )
		ADLASSERT(0);

	if( FAILED( InitDevice() ) )
	{
		deallocate();
		ADLASSERT(0);
	}

	prepareGlobalResources();

	ShowWindow( g_hWnd, nCmdShow );
}

void AdlGraphics::renderLoop()
{
	MSG msg = {0};
	while( WM_QUIT != msg.message )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			render();
		}
	}
}

void AdlGraphics::deallocate()
{
	finishDemo();
	g_font.deallocate();

	DeviceRenderTargetDX11::deleteRenderTarget( g_deviceData, g_renderTarget );
	DeviceRenderTargetDX11::deleteRenderTarget( g_deviceData, g_depthStencil );

	if( g_postEffect ) delete g_postEffect;

	if( g_appVertexBuffer ) delete g_appVertexBuffer;
	if( g_appIndexBuffer ) delete g_appIndexBuffer;

	if( g_appDebugVertexBuffer ) delete g_appDebugVertexBuffer;
	if( g_appDebugIndexBuffer ) delete g_appDebugIndexBuffer;

	if( g_rasterizerStateSolid ) g_rasterizerStateSolid->Release();
	if( g_rasterizerStateWireframe ) g_rasterizerStateWireframe->Release();

	if( g_depthTestOn ) g_depthTestOn->Release();
	if( g_depthTestOff ) g_depthTestOff->Release();

	if( g_defaultSampler )g_defaultSampler->Release();
	if( g_blendState ) g_blendState->Release();

	g_deviceData->deallocate( &g_constBuffer );
 
	ShaderUtilsDX11::deleteShader( g_defaultVertexShader );
	ShaderUtilsDX11::deleteShader( g_defaultPixelShader );
	ShaderUtilsDX11::deleteShader( g_quadVertexShader );
	ShaderUtilsDX11::deleteShader( g_quadPixelShader );
	ShaderUtilsDX11::deleteShader( g_bufferToRTPixelShader );
	ShaderUtilsDX11::deleteShader( g_textureMapWColorPixelShader );

	ShaderUtilsDX11::deleteShader( g_pointSpriteVertexShader );
	ShaderUtilsDX11::deleteShader( g_pointSpriteGeometryShader );
	ShaderUtilsDX11::deleteShader( g_pointSpritePixelShader );

	ID3D11Device* d = g_deviceData->m_device;

	if( g_deviceData->m_swapChain ) g_deviceData->m_swapChain->Release();
	g_deviceData->m_swapChain = 0;
	DeviceUtils::deallocate( g_deviceData );

	if(0)
	{
		ID3D11Debug* pDebug; d->QueryInterface( __uuidof(ID3D11Debug), (void**)(&pDebug) ); 
		pDebug->ReportLiveDeviceObjects( D3D11_RLDO_SUMMARY );
		pDebug->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL );
		pDebug->Release();
	}
}


LRESULT CALLBACK AdlGraphics::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;
	
	OnMouse( message, wParam & MK_SHIFT );

    switch( message )
    {
		case WM_SIZE:
			{
				int newWidth = LOWORD(lParam);
				int newHeight = HIWORD(lParam);
				if( g_wWidth != newWidth || g_wHeight != newHeight )
				{
					g_wWidth = newWidth;
					g_wHeight = newHeight;
					OnResize();
				}
			}
			break;
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;
		case WM_KEYDOWN:
			OnKeyboard( wParam );//+32 );
			break;
        case WM_DESTROY:
			finishDemo();
            PostQuitMessage( 0 );
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
	};

	return DefWindowProc( hWnd, message, wParam, lParam ); 
}

#define MOUSE_TRANSLATE 1
#define MOUSE_SCALE (1<<1)
#define MOUSE_ROTATE (1<<2)

u32 AdlGraphics::g_mouseFlags = 0;

void AdlGraphics::OnMouse( UINT message, UINT shift )
{
	if( message == WM_RBUTTONDOWN )
		g_mouseFlags |= MOUSE_TRANSLATE;
	else if( message == WM_RBUTTONUP )
		g_mouseFlags ^= MOUSE_TRANSLATE;

	if( message == WM_RBUTTONDOWN && shift )
		g_mouseFlags |= MOUSE_SCALE;
	else if( message == WM_RBUTTONUP && shift )
		g_mouseFlags ^= MOUSE_SCALE;
	
	if( message == WM_LBUTTONDOWN )
		g_mouseFlags |= MOUSE_ROTATE;
	else if( message == WM_LBUTTONUP )
		g_mouseFlags ^= MOUSE_ROTATE;



	float dx, dy;
	POINT ptCurMousePos;
	GetCursorPos( &ptCurMousePos );
	int x = (int)ptCurMousePos.x;
	int y = (int)ptCurMousePos.y;
	dx = (float)(x - g_mouseX);
	dy = (float)(y - g_mouseY);

	if( g_mouseFlags & MOUSE_ROTATE )
	{
		g_rotationX += dy * 0.2f/90.f;
		g_rotationY += dx * 0.2f/90.f;
	}
		
	if( g_mouseFlags & MOUSE_SCALE )
	{
		g_translationZ -= dy * 0.01f;
	}
	else if( g_mouseFlags & MOUSE_TRANSLATE )
	{
		g_translationX += dx * 0.005f;
		g_translationY -= dy * 0.005f;
	}

	g_mouseX = x;
	g_mouseY = y;
}

void AdlGraphics::OnKeyboard( UINT nChar )
{
	keyListenerDemo((unsigned char)nChar);

    {
        switch( nChar )
        {
		case 'A':
			g_autoMove = !g_autoMove;
			break;
		case 'R':
			resetDemo();
			break;
		case ' ':
//		case VK_SPACE:
			step();
			break;
		case 'L':
			g_demo->m_enableLighting = !g_demo->m_enableLighting;
			break;
        case VK_F1:
			break;
		case 'Q':
		case VK_ESCAPE:
			{
				finishDemo();
				PostQuitMessage( 0 );
			}
			break;
		default:
			break;
        }
    }
}

void AdlGraphics::renderText()
{
	g_deviceData->m_context->OMSetDepthStencilState( g_depthTestOff, 0 );
	float factor[] = {0,0,0,0};
	g_deviceData->m_context->OMSetBlendState( g_blendState, factor, 0xFFFFFFFF  );

	g_font.reset();
	{
		IDXGIAdapter* adapter = getAdapterFromDevice( g_deviceData );
		DXGI_ADAPTER_DESC adapterDesc;
		adapter->GetDesc( &adapterDesc );

		char name[128];
		wcstombs( name, adapterDesc.Description, 128 );

		g_font.printf( name, make_float4(1,0,0,1) );

		adapter->Release();
	}

	for(int i=0; i<g_demo->m_nTxtLines; i++)
	{
		g_font.printf( g_demo->m_txtBuffer[i], make_float4(1.f)-g_demo->m_backgroundColor );
	}

	g_deviceData->m_context->OMSetBlendState( NULL, factor, 0xFFFFFFFF  );
	g_deviceData->m_context->OMSetDepthStencilState( g_depthTestOn, 0 );
}

void AdlGraphics::render()
{
	DeviceUtils::waitForCompletion( g_deviceData );

	{//	step
		if( g_demo->m_enableLighting )
			g_deviceData->m_context->RSSetState( g_rasterizerStateSolid );
		else
			g_deviceData->m_context->RSSetState( g_rasterizerStateWireframe );
		if( g_autoMove )
			step();
	}

	XMMATRIX cView = g_View;
	{
		g_World = XMMatrixIdentity();
		XMMATRIX trans = XMMatrixTranslation( g_translationX, g_translationY, g_translationZ );
		XMMATRIX rotX = XMMatrixTranspose( XMMatrixRotationX( g_rotationX ) );
		XMMATRIX rotY = XMMatrixTranspose( XMMatrixRotationY( g_rotationY ) );
		cView = XMMatrixMultiply( XMMatrixMultiply( XMMatrixMultiply( rotY, rotX ), trans), cView );
	}

	g_ViewTr = XMMatrixTranspose( cView );
	g_ProjectionTr = XMMatrixTranspose( g_Projection );


	g_appDebugVertexBuffer->reset();
	g_appDebugIndexBuffer->reset();
	g_debugRenderObj.clear();

	float ClearColor[4] = { g_demo->m_backgroundColor.x, g_demo->m_backgroundColor.y, g_demo->m_backgroundColor.z, g_demo->m_backgroundColor.w };

	g_deviceData->m_context->OMSetRenderTargets( 1, &g_renderTarget.m_renderTarget, g_depthStencil.m_depthStencilView );
	g_deviceData->m_context->ClearRenderTargetView( g_renderTarget.m_renderTarget, ClearColor );
	g_deviceData->m_context->ClearDepthStencilView( g_depthStencil.m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 );

	if( g_demo->m_enableLighting && g_demo->m_enablePostEffect )
		g_postEffect->renderPre();
	g_demo->renderPre();

	g_demo->render();

	if( g_demo->m_enableAlphaBlending )
	{
		float factor[] = {0,0,0,0};
		g_deviceData->m_context->OMSetBlendState( g_blendState, factor, 0xFFFFFFFF  );
	}

	for(int ie=0; ie<2; ie++)
	{
		Array<RenderObject>* objArray = (ie==0)? &g_renderObj: &g_debugRenderObj;

		dispatchRenderList( g_deviceData, objArray, &g_ViewTr, &g_ProjectionTr );
	}

	if( g_demo->m_enableAlphaBlending )
	{
		float factor[] = {0,0,0,0};
		g_deviceData->m_context->OMSetBlendState( NULL, factor, 0xFFFFFFFF  );
	}

	g_deviceData->m_context->OMSetRenderTargets( 1, &g_renderTarget.m_renderTarget, g_depthStencil.m_depthStencilView );

	g_demo->renderPost();
	if( g_demo->m_enableLighting && g_demo->m_enablePostEffect )
		g_postEffect->renderPost();



	g_appDebugVertexBuffer->reset();
	g_appDebugIndexBuffer->reset();
	g_debugRenderObj.clear();
	{	//	UI
		g_deviceData->m_context->OMSetDepthStencilState( g_depthTestOff, 0 );
		float factor[] = {0,0,0,0};
		g_deviceData->m_context->OMSetBlendState( g_blendState, factor, 0xFFFFFFFF  );

		g_demo->drawThreadProfile();

		dispatchRenderList( g_deviceData, &g_debugRenderObj, &g_ViewTr, &g_ProjectionTr );

		if( !g_writeBmp )
			renderText();

		g_deviceData->m_context->OMSetBlendState( NULL, factor, 0xFFFFFFFF  );
		g_deviceData->m_context->OMSetDepthStencilState( g_depthTestOn, 0 );
	}

	g_deviceData->m_swapChain->Present( 0, 0 );
}



