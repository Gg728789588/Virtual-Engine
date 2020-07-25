#include "DescHeapPool.h"

DescHeapPool::DescHeapPool(
	uint size, uint initCapacity) : size(size), capacity(initCapacity)
{
	poolValue.reserve(initCapacity);
	arr.reserve(10);
}

void DescHeapPool::Add(ID3D12Device* device)
{
	ObjectPtr<DescriptorHeap>& newDesc = arr.emplace_back(
		ObjectPtr<DescriptorHeap>::NewObject(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, size * capacity, true));
	DescriptorHeap* heap = newDesc;
	for (uint i = 0; i < capacity; ++i)
	{
		poolValue.push_back(DescHeapElement(heap, i * size));
	}
}

DescHeapElement DescHeapPool::Get(ID3D12Device* device)
{
	if (poolValue.empty())
		Add(device);
	auto ite = poolValue.end() - 1;
	DescHeapElement result = *ite;
	poolValue.erase(ite);
	return result;
}
void DescHeapPool::Return(const DescHeapElement& target)
{
	poolValue.push_back(target);
}