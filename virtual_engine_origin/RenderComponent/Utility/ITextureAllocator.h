#pragma once
#include "../../Common/d3dUtil.h"
#include "../../Common/MetaLib.h"
#include "../ITexture.h"
class ITextureAllocator
{
public:
	virtual void AllocateTextureHeap(
		ID3D12Device* device,
		DXGI_FORMAT format,
		uint32_t width,
		uint32_t height,
		uint32_t depthSlice,
		TextureDimension dimension,
		uint32_t mipCount,
		ID3D12Heap** heap, uint64_t* offset,
		ITexture* currentPtr) = 0;
	virtual void ReturnTexture(ITexture* tex) = 0;
	virtual ~ITextureAllocator() {}
};