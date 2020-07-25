//***************************************************************************************
// d3dUtil.h by Frank Luna (C) 2015 All Rights Reserved.
//
// General helper code.
//***************************************************************************************

#pragma once
#define NOMINMAX
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
#include "vector.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cassert>
#include "d3dx12.h"
#include "vstring.h"

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
using uint64 = uint64_t;
using int64 = int64_t;
#include "MathHelper.h"
#include "HashMap.h"
#include "DXMath/DXMath.h"

enum ResourceReadWriteState : bool
{
	Write = false,
	Read = true
};
INLINE void d3dSetDebugName(IDXGIObject* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
INLINE void d3dSetDebugName(ID3D12Device* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}
INLINE void d3dSetDebugName(ID3D12DeviceChild* obj, const char* name)
{
    if(obj)
    {
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
    }
}

INLINE vengine::wstring AnsiToWString(const vengine::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return vengine::wstring(buffer);
}

/*
#if defined(_DEBUG)
    #ifndef Assert
    #define Assert(x, description)                                  \
    {                                                               \
        static bool ignoreAssert = false;                           \
        if(!ignoreAssert && !(x))                                   \
        {                                                           \
            Debug::AssertResult result = Debug::ShowAssertDialog(   \
            (L#x), description, AnsiToWString(__FILE__), __LINE__); \
        if(result == Debug::AssertIgnore)                           \
        {                                                           \
            ignoreAssert = true;                                    \
        }                                                           \
                    else if(result == Debug::AssertBreak)           \
        {                                                           \
            __debugbreak();                                         \
        }                                                           \
        }                                                           \
    }
    #endif
#else
    #ifndef Assert
    #define Assert(x, description) 
    #endif
#endif 		
    */

class d3dUtil
{
public:

    static bool IsKeyDown(int vkeyCode);

    static vengine::string ToString(HRESULT hr);

    static UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        // Constant buffers must be a multiple of the minimum hardware
        // allocation size (usually 256 bytes).  So round up to nearest
        // multiple of 256.  We do this by adding 255 and then masking off
        // the lower 2 bytes which store all bits < 256.
        // Example: Suppose byteSize = 300.
        // (300 + 255) & ~255
        // 555 & ~255
        // 0x022B & ~0x00ff
        // 0x022B & 0xff00
        // 0x0200
        // 512
        return (byteSize + 255) & ~255;
    }

    static uint64 CalcPlacedOffsetAlignment(uint64 offset)
    {
        return (offset + 65535) & ~65535;
    }

    static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const vengine::wstring& filename);
	static std::array<const CD3DX12_STATIC_SAMPLER_DESC, 12> GetStaticSamplers();
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const void* initData,
        UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const vengine::string& filename,
		const D3D_SHADER_MACRO* defines,
		const vengine::string& entrypoint,
		const vengine::string& target);
    static Microsoft::WRL::ComPtr<ID3DBlob> ReadCompiledShader(
        const vengine::string& filename);
};

class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const vengine::wstring& functionName, const vengine::wstring& filename, int lineNumber);

    vengine::wstring ToString()const;

    HRESULT ErrorCode = S_OK;
    vengine::wstring FunctionName;
    vengine::wstring Filename;
    int LineNumber = -1;
};


#ifndef ThrowIfFailed
#ifdef NDEBUG
#define ThrowIfFailed(x) (x)
#else
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    vengine::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif
#endif
#ifndef ThrowHResult
#define ThrowHResult(hr__, x) \
{\
    vengine::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}

#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif