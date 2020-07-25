#include "DefaultBufferAllocator.h"
#include "D3D12MemoryAllocator/D3D12MemAlloc.h"
#include "../IGPUResource.h"
DefaultBufferAllocator::DefaultBufferAllocator(ID3D12Device* device, IDXGIAdapter* adapter, bool isSingleThread) :
	allocatedTexs(32),
	isSingleThread(isSingleThread)
{
	D3D12MA::ALLOCATOR_DESC desc;
	desc.Flags = D3D12MA::ALLOCATOR_FLAGS::ALLOCATOR_FLAG_SINGLETHREADED;
	desc.pAdapter = adapter;
	desc.pAllocationCallbacks = nullptr;
	desc.pDevice = device;
	desc.PreferredBlockSize = 0;
	D3D12MA::CreateAllocator(&desc, &allocator);
	if (!isSingleThread)
	{
		mtx.New();
	}
}

void DefaultBufferAllocator::AllocateTextureHeap(
	ID3D12Device* device,
	uint64_t targetSizeInBytes,
	D3D12_HEAP_TYPE heapType,
	ID3D12Heap** heap, uint64_t* offset,
	uint instanceID)
{
	D3D12MA::ALLOCATION_DESC desc;
	desc.HeapType = heapType;
	desc.Flags = D3D12MA::ALLOCATION_FLAGS::ALLOCATION_FLAG_NONE;
	desc.ExtraHeapFlags = D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	desc.CustomPool = nullptr;
	D3D12_RESOURCE_ALLOCATION_INFO info;
	info.Alignment = 65536;
	info.SizeInBytes = d3dUtil::CalcPlacedOffsetAlignment(targetSizeInBytes);
	D3D12MA::Allocation* alloc;
	if (!isSingleThread)
	{
		mtx->lock();
	}
	allocator->AllocateMemory(&desc, &info, &alloc);
	allocatedTexs.Insert(instanceID, alloc);
	if (!isSingleThread)
	{
		mtx->unlock();
	}
	*heap = alloc->GetHeap();
	*offset = alloc->GetOffset();
}

DefaultBufferAllocator::~DefaultBufferAllocator()
{
	if (allocator) allocator->Release();
}

void DefaultBufferAllocator::ReturnBuffer(uint instanceID)
{
	if (!isSingleThread)
	{
		mtx->lock();
	}
	auto ite = allocatedTexs.Find(instanceID);
	if (!ite)
	{
		if (!isSingleThread)
		{
			mtx->unlock();
		}
		return;
	}
	ite.Value()->Release();
	allocatedTexs.Remove(ite);
	if (!isSingleThread)
	{
		mtx->unlock();
	}
}