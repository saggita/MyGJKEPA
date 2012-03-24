#ifndef DEVICE_DRAW_H
#define DEVICE_DRAW_H

#include <Adl/Adl.h>
#include <AdlPrimitives/Math/Math.h>
//#include <Common/Geometry/Aabb.h>


#if defined(DX11RENDER)
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>

#include <AdlGraphics/DeviceDrawDx11.inl>


#define pxDrawLine(a,b,color) drawLine(*(XMFLOAT4*)&(a), *(XMFLOAT4*)&(b), *(XMFLOAT4*)&(color))
#define pxDrawLineList(vtx,idx,nVtx,nIdx,color) drawLineList( (XMFLOAT4*)vtx, idx, nVtx, nIdx, *(XMFLOAT4*)&(color) )
#define pxDrawPoint(a,color) drawPoint(*(XMFLOAT4*)&(a), *(XMFLOAT4*)&(color))
#define pxDrawPointList(vtx,color,nVtx) drawPointList((XMFLOAT4*)(vtx), (XMFLOAT4*)(color), nVtx)
#define pxDrawPointSprite(vtx,color,radius,nVtx) drawPointSprite((XMFLOAT4*)(vtx), (XMFLOAT4*)(color), (XMFLOAT2*)(radius), nVtx)
#define pxDrawPointSprite1(vtxBuffer, colorBuffer, nVtx, radius) drawPointSprite(vtxBuffer, colorBuffer, nVtx, radius)

#define pxDrawPointListTransformed(vtx,color,nVtx,translation,quaternion) drawPointListTransformed((XMFLOAT4*)(vtx), (XMFLOAT4*)(color), nVtx,translation,quaternion);
#define pxDrawTriangle(a,b,c,color) drawTriangle(*(XMFLOAT4*)&(a), *(XMFLOAT4*)&(b), *(XMFLOAT4*)&(c), *(XMFLOAT4*)&(color))
#define pxDrawTriangleList(vtx,idx,nVtx,nIdx,color) drawTriangleList((XMFLOAT4*)(vtx), (idx), nVtx, nIdx, *(XMFLOAT4*)&(color))
#define pxDrawTriangleList1(vtx,idx,nVtx,nIdx,color) drawTriangleList1((XMFLOAT4*)(vtx), (idx), nVtx, nIdx, (XMFLOAT4*)color)
#define pxDrawTriangleListNormal(vtx,vtxNormal,idx,nVtx,nIdx,color) drawTriangleList((XMFLOAT4*)vtx, (XMFLOAT4*)vtxNormal, idx, nVtx, nIdx, *(XMFLOAT4*)&color)
#define pxDrawTriangleListTessellated(vtx,vtxNormal,idx,nVtx,nIdx,color,translation,quaternion,vtxShader,hShader,dShader,pShader) drawTriangleListTessellated((XMFLOAT4*)vtx, (XMFLOAT4*)vtxNormal, idx, nVtx, nIdx, *(XMFLOAT4*)&color,translation,quaternion,vtxShader,hShader,dShader,pShader)

#define pxDrawTriangleListTransformed(vtx,vtxNormal,idx,nVtx,nIdx,color,translation,quaternion) drawTriangleListTransformed<true>((XMFLOAT4*)vtx, (XMFLOAT4*)vtxNormal, idx, nVtx, nIdx, *(XMFLOAT4*)&color,translation,quaternion)
#define pxReleaseDrawTriangleListTransformed(vtx,vtxNormal,idx,nVtx,nIdx,color,translation,quaternion) drawTriangleListTransformed<false>((XMFLOAT4*)vtx, (XMFLOAT4*)vtxNormal, idx, nVtx, nIdx, *(XMFLOAT4*)&color,translation,quaternion)

#define DevicePSShader DeviceShaderDX11
#define pxCreatePixelShader(deviceData, shaderPath, profile, shaderOut) shaderOut = ShaderUtilsDX11::createPixelShader(deviceData, shaderPath, profile)
#define pxDeleteShader(shader) ShaderUtilsDX11::deleteShader(shader)
#define pxSetPixelShader(pShader) g_debugRenderObj[g_debugRenderObj.getSize()-1].m_pixelShader = pShader;

#define pxDrawText(txt,pos)
#define pxDrawAabb(ma,mi, c) drawAabb(ma,mi, c)

//	todo. this doesn't work. It should insert command but not calling this function here as everything will be batched. 
#define pxClearDepthStencil
//#define pxClearDepthStencil g_deviceData.m_context->ClearDepthStencilView( g_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0 )

#else
#include <glut.h>
#include <AdlGraphics/DeviceDrawGL.inl>
#define pxDrawLine(a,b,color) drawLine(a, b, color)
#define pxDrawLineList(vtx,idx,nVtx,nIdx,color) drawLineList(vtx,idx,nVtx,nIdx,color)
#define pxDrawPoint(a,color) drawPoint(a, color)
#define pxDrawPointList(vtx,color,nVtx) drawPointList(vtx,color,nVtx);
#define pxDrawPointSprite(vtx,color,radius, nVtx) drawPointList(vtx,color,nVtx)
#define pxDrawPointListTransformed(vtx,color,nVtx,translation,quaternion) drawPointListTransformed(vtx,color,nVtx,translation,quaternion);
#define pxDrawTriangle(a,b,c,color) drawTriangle(a,b,c,color)
#define pxDrawTriangleList(vtx,idx,nVtx,nIdx,color) drawTriangleList(vtx,idx,nVtx,nIdx,color)
#define pxDrawTriangleList1(vtx,idx,nVtx,nIdx,color) drawTriangleList1(vtx,idx,nVtx,nIdx,color)
#define pxDrawTriangleListNormal(vtx,vtxNormal,idx,nVtx,nIdx,color) drawTriangleList(vtx,vtxNormal,idx,nVtx,nIdx,color)
#define pxDrawTriangleListTessellated(vtx,vtxNormal,idx,nVtx,nIdx,color,translation,quaternion,vtxShader,hShader,dShader,pShader) drawTriangleListTransformed(vtx,vtxNormal,idx,nVtx,nIdx,color,translation,quaternion)
#define pxDrawTriangleListTransformed(vtx,vtxNormal,idx,nVtx,nIdx,color,translation,quaternion) drawTriangleListTransformed(vtx,vtxNormal,idx,nVtx,nIdx,color,translation,quaternion)

#define pxDrawText(txt,pos) glDraw3DStrings(txt, pos)
#define pxDrawAabb(aabb, c) drawAabb(aabb, c)

#define DevicePSShader int
#define pxCreatePixelShader(deviceData, shaderPath, profile, shaderOut) {shaderOut;}
#define pxDeleteShader(shader) {shader;}
#define pxSetPixelShader(pShader) {pShader;}

#define pxClearDepthStencil glClear( GL_DEPTH_BUFFER_BIT )

#endif

#endif

