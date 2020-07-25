#pragma once
#include "../../Common/d3dUtil.h"
#include "../../Common/MetaLib.h"
class IGPUResource;
class IBufferAllocator
{
public:
	virtual void AllocateTextureHeap(
		ID3D12Device* device,
		uint64_t targetSizeInBytes,
		D3D12_HEAP_TYPE heapType,
		ID3D12Heap** heap, uint64_t* offset,
		uint instanceID) = 0;
	virtual void ReturnBuffer(uint instanceID) = 0;
	virtual ~IBufferAllocator() {}
};