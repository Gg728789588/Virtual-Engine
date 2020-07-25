#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/MetaLib.h"
#include "DescriptorHeap.h"
struct DescHeapElement
{
	DescriptorHeap* const heap;
	uint const offset;
	DescHeapElement() : heap(nullptr), offset(0) {}
	DescHeapElement(DescriptorHeap* const heap,
	uint const offset) : heap(heap), offset(offset) {}
	DescHeapElement(const DescHeapElement& ele) :
		heap(ele.heap), offset(ele.offset) {}
	void operator=(const DescHeapElement& ele)
	{
		new (this) DescHeapElement(ele);
	}
};
class DescHeapPool
{
private:
	vengine::vector<ObjectPtr<DescriptorHeap>> arr;
	ArrayList<DescHeapElement> poolValue;
	uint capacity;
	uint size;
	void Add(ID3D12Device* device);
public:
	DescHeapPool(uint size, uint initCapacity);
	DescHeapElement Get(ID3D12Device* device);
	void Return(const DescHeapElement& target);
};