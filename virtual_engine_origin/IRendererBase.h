#pragma once
#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <memory>
//#include <algorithm>

#include <array>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include "Common/GameTimer.h"
typedef DirectX::XMFLOAT2 float2;
typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMUINT2 uint2;
typedef DirectX::XMUINT3 uint3;
typedef DirectX::XMUINT4 uint4;
typedef DirectX::XMINT2 int2;
typedef DirectX::XMINT3 int3;
typedef DirectX::XMINT4 int4;
typedef uint32_t uint;
typedef DirectX::XMFLOAT4X4 float4x4;
typedef DirectX::XMFLOAT3X3 float3x3;
typedef DirectX::XMFLOAT3X4 float3x4;
typedef DirectX::XMFLOAT4X3 float4x3;
class GameTimer;
class D3DAppDataPack;
struct D3DAppSettings
{
	DXGI_FORMAT backBufferFormat;
	UINT64 mCurrentFence;
	int mCurrBackBuffer;
	HINSTANCE mhAppInst; // application instance handle
	bool      mAppPaused;  // is the application paused?
	bool      mMinimized;  // is the application minimized?
	bool      mMaximized;  // is the application maximized?
	bool      mResizing;   // are the resize bars being dragged?
	

	UINT mRtvDescriptorSize;
	UINT mDsvDescriptorSize;
	UINT mCbvSrvUavDescriptorSize;

	wchar_t* windowInfo;
	int mClientWidth;
	int mClientHeight;
};
struct D3DAppDataPack
{
	
	//static constexpr D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;	
	Microsoft::WRL::ComPtr < ID3D12Device> md3dDevice;
	IDXGIAdapter1* adapter;
	HWND      mhMainWnd;
	D3DAppSettings settings;
};
class IRendererBase
{
public:
	virtual bool __cdecl Initialize(D3DAppDataPack&) = 0;
	virtual void __cdecl Dispose(D3DAppDataPack&) = 0;
	virtual void __cdecl OnResize(D3DAppDataPack& pack) = 0;
	virtual bool __cdecl Draw(D3DAppDataPack&, GameTimer&) = 0;
	virtual ~IRendererBase() {}
};

IRendererBase* __cdecl GetRenderer();//Declare of the entry of renderer