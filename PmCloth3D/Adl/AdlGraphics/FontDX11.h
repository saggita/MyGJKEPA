#pragma once

class FontDX11
{
	public:
		FontDX11():m_fontSRV(0){}
		void allocate( const DeviceDX11* dd, char* path );
		void deallocate(){ if(m_fontSRV) m_fontSRV->Release(); m_fontSRV = 0; }
		void reset();
		void printf( char* txt, const float4& color );
		void printf( char* txt, const float4& pos, const float4& color );

	public:
		const DeviceDX11* m_deviceData;
		ID3D11ShaderResourceView* m_fontSRV;

		float2 m_fontSize;
		int m_nLines;
};


__inline
void FontDX11::allocate( const DeviceDX11* dd, char* path )
{
	m_deviceData = dd;
	m_fontSize = make_float2( 15.f, 42.f );

	const int LINE_CAPACITY = 256;
	HRESULT hr = S_OK;
	WCHAR str[Demo::LINE_CAPACITY];
	MultiByteToWideChar(CP_ACP,0,path,-1, str, min2((size_t)LINE_CAPACITY, strlen(path)+1));

	D3DX11CreateShaderResourceViewFromFile( dd->m_device, str, NULL, NULL, &m_fontSRV, &hr);
	ADLASSERT( hr == S_OK );
}

__inline
void FontDX11::reset()
{
	m_nLines = 0;
}

void FontDX11::printf( char* txt, const float4& color )
{
	const DeviceDX11* dd = (const DeviceDX11*) m_deviceData;

	dd->m_context->PSSetShaderResources( 0, 1, &m_fontSRV );

	{
		float margin = 0.025f;

		{
			float factor[] = {0,0,0,0};
			dd->m_context->OMSetBlendState( g_blendState, factor, 0xFFFFFFFF );
		}

		const float charTxtSizeX = 0.010526315f;

		float fontSSizeX = m_fontSize.x / g_wWidth;
		float fontSSizeY = m_fontSize.y / g_wHeight;

		float height = 1.f-(35.f/g_wHeight)*(1.f+m_nLines);
		for(int i=0; i<(int)strlen(txt); i++)
		{
			int txtIdx = txt[i]-32;
			float4 minCrd = make_float4(charTxtSizeX*txtIdx,0,0);
			float4 maxCrd = make_float4(minCrd.x+charTxtSizeX,1,0);

			RenderObject quad = createQuad1(make_float4(margin+-1+fontSSizeX*i,-margin+height,0), make_float4(fontSSizeX,fontSSizeY,0), minCrd, maxCrd,
				color.x, color.y, color.z, color.w);
			quad.m_pixelShader = g_textureMapWColorPixelShader;
			quad.m_vertexShader = g_quadVertexShader;
			dd->m_context->PSSetSamplers( 0, 1, &g_defaultSampler );
			quad.render( dd, g_constBuffer.getBuffer() );
		}
	}

	ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
	dd->m_context->PSSetShaderResources( 0, 1, ppSRVNULL );

	{
		float factor[] = {0,0,0,0};
		dd->m_context->OMSetBlendState( NULL, factor, 0xFFFFFFFF  );
	}
	m_nLines++;
}

void FontDX11::printf( char* txt, const float4& pos, const float4& color )
{
	const DeviceDX11* dd = (const DeviceDX11*) m_deviceData;

	dd->m_context->PSSetShaderResources( 0, 1, &m_fontSRV );

	{
		float margin = 0.025f;

		{
			float factor[] = {0,0,0,0};
			dd->m_context->OMSetBlendState( g_blendState, factor, 0xFFFFFFFF );
		}

		const float charTxtSizeX = 0.010526315f;

		float fontSSizeX = m_fontSize.x / g_wWidth;
		float fontSSizeY = m_fontSize.y / g_wHeight;

		float height = 1.f-(35.f/g_wHeight)*(1.f+m_nLines);
		for(int i=0; i<(int)strlen(txt); i++)
		{
			int txtIdx = txt[i]-32;
			float4 minCrd = make_float4(charTxtSizeX*txtIdx,0,0);
			float4 maxCrd = make_float4(minCrd.x+charTxtSizeX,1,0);

			RenderObject quad = createQuad1(make_float4(fontSSizeX*i,0,0)+pos, make_float4(fontSSizeX,fontSSizeY,0), minCrd, maxCrd,
				color.x, color.y, color.z, color.w);
			quad.m_pixelShader = g_textureMapWColorPixelShader;
//			quad.m_vertexShader = g_quadVertexShader;
			dd->m_context->PSSetSamplers( 0, 1, &g_defaultSampler );

			{
				quad.m_matrix.m_view = g_ViewTr;
				quad.m_matrix.m_projection = g_ProjectionTr;
				Kernel kernel;
				Launcher launcher( g_deviceData, &kernel );
				launcher.setConst<ConstantBuffer>( g_constBuffer, quad.m_matrix );
			}

			quad.render( dd, g_constBuffer.getBuffer() );
		}
	}

	ID3D11ShaderResourceView* ppSRVNULL[16] = { 0 };
	dd->m_context->PSSetShaderResources( 0, 1, ppSRVNULL );

	{
		float factor[] = {0,0,0,0};
		dd->m_context->OMSetBlendState( NULL, factor, 0xFFFFFFFF  );
	}
	m_nLines++;
}
