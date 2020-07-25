#pragma once
#include "IBufferAllocator.h"
namespace D3D12MA
{
	class Allocator;
	class Allocation;
}
class DefaultBufferAllocator final: public IBufferAllocator
{
private:
	HashMap<uint, D3D12MA::Allocation*> allocatedTexs;
	D3D12MA::Allocator* allocator;
	StackObject<std::mutex, true> mtx;
	bool isSingleThread;
public:
	DefaultBufferAllocator(ID3D12Device* device, IDXGIAdapter* adapter, bool isSingleThread = false);
	virtual void AllocateTextureHeap(
		ID3D12Device* device,
		uint64_t targetSizeInBytes,
		D3D12_HEAP_TYPE heapType,
		ID3D12Heap** heap, uint64_t* offset,
		uint instanceID);
	virtual void ReturnBuffer(uint instanceID);
	~DefaultBufferAllocator();
};