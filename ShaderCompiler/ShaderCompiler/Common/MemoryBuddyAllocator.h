#pragma once
#include "BinaryAllocator.h"
#include <memory>
#include "MetaLib.h"
#include "HashMap.h"
class MemoryBuddyAllocator
{
private:
	void* ptr;
	uint64 atomicChunk;
	uint64 fullSize;
	vengine::vector<std::unique_ptr<BinaryAllocator>> allocs;
	HashMap<void*, std::pair<uint, BinaryAllocator::AllocatedChunkHandle>> handlesDict;
	uint fullLayer;
	static uint BitCount(uint64 target);
public:
	//pow2 setting
	//default align as 16 bytes (1 << 4)
	MemoryBuddyAllocator(
		uint maximumAllocLayer,
		uint alignLayer = 4);
	~MemoryBuddyAllocator();
	void* Allocate(uint64 size);
	void Free(void* ptr);
};

