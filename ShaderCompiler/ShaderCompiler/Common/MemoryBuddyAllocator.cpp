#include "MemoryBuddyAllocator.h"
#include "Memory.h"
#include <algorithm>
uint MemoryBuddyAllocator::BitCount(uint64 target)
{
	if (!target) return 0;
	target--;
	for (uint i = 0; i < 63; ++i)
	{
		if (!target) return i;
		target >>= 1;
	}
}
MemoryBuddyAllocator::MemoryBuddyAllocator(
	uint maximumAllocLayer,
	uint align) : handlesDict(53)
{
	maximumAllocLayer = std::max({ maximumAllocLayer, align });
	atomicChunk = (uint64)1 << (uint64)align;
	fullSize = (uint64)1 << (uint64)maximumAllocLayer;
	fullLayer = maximumAllocLayer - align + 1;
	allocs.reserve(10);
	allocs.emplace_back(
		new BinaryAllocator(
			fullLayer));
	ptr = aligned_malloc(fullSize, atomicChunk);
}

MemoryBuddyAllocator::~MemoryBuddyAllocator()
{
	aligned_free(ptr);
}

void* MemoryBuddyAllocator::Allocate(uint64 size)
{
	if (!size || size > fullSize) return nullptr;
	uint64 chunk = size / atomicChunk;
	if (size % atomicChunk) chunk++;
	uint64 layer = fullLayer - BitCount(chunk) - 1;
	BinaryAllocator::AllocatedChunkHandle handle;
	uint allocIndex = 0;
	{
		for (uint i = 0; i < allocs.size(); ++i)
		{
			if (allocs[i]->TryAllocate(layer, handle))
			{
				allocIndex = i;
				goto CONTINUE_ALLOC;
			}
		}
		allocIndex = allocs.size();
		std::unique_ptr< BinaryAllocator>& newAlloc = allocs.emplace_back(new BinaryAllocator(fullLayer));
		newAlloc->TryAllocate(layer, handle);
	}
CONTINUE_ALLOC:
	uint64 offset = handle.GetOffset() * atomicChunk;
	void* allocatedPtr = (void*)((uint64)ptr + offset);
	handlesDict.Insert(
		allocatedPtr,
		std::pair<uint, BinaryAllocator::AllocatedChunkHandle>(allocIndex, handle)
	);
	return allocatedPtr;
}
void MemoryBuddyAllocator::Free(void* ptr)
{
	auto ite = handlesDict.Find(ptr);
	if (!ite) return;
	allocs[ite.Value().first]->Return(ite.Value().second);
	handlesDict.Remove(ite);
}