#pragma once
#include <stdint.h>

#include "../Common/HashMap.h"
#include "../Common/vector.h"
#include "../Common/ArrayList.h"
typedef uint32_t uint;
typedef uint64_t uint64;


class BinaryAllocator
{
public:
	struct BinaryTreeNode
	{
		uint64 size;
		uint64 offset;
		uint layer;
		bool isRight() const noexcept
		{
			return (offset / size) % 2;
		}
		BinaryTreeNode() {}
		BinaryTreeNode(uint64 size, uint64 offset, uint layer)
		{
			this->size = size;
			this->layer = layer;
			this->offset = offset;
		}
		bool operator==(const BinaryTreeNode& node) const noexcept
		{
			return size == node.size && offset == node.offset && layer == node.layer;
		}

		bool operator!=(const BinaryTreeNode& node) const noexcept
		{
			return !operator==(node);
		}
	};
	struct AllocatedChunkHandleHash;
	struct AllocatedChunkHandle
	{
		friend class BinaryAllocator;
		friend class AllocatedChunkHandleHash;
	private:
		BinaryTreeNode node;
	public:
		uint64 GetSize() const noexcept { return node.size; }
		uint64 GetOffset() const noexcept { return node.offset; }
	};
	struct AllocatedChunkHandleHash
	{
		inline size_t operator()(const AllocatedChunkHandle& handle) const noexcept;
	};
private:
	struct BinaryCoord
	{
		uint layer;
		uint index;
	};
	struct BinaryTreeNodeHash
	{
		size_t operator()(const BinaryTreeNode& node) const noexcept
		{
			return
				(std::hash<int64_t>::_Do_hash(node.size) << 8) ^
				(std::hash<uint64>::_Do_hash(node.offset) << 4) ^
				(std::hash<uint>::_Do_hash(node.layer));
		}
	};
	HashMap<BinaryTreeNode, BinaryCoord, BinaryTreeNodeHash> singleNodeCacheDict;
	vengine::vector<ArrayList<BinaryTreeNode>> usefulNodes;
	uint64_t rootChunkSize;
	void ReturnChunk(const BinaryTreeNode& node);
	void AddNode(const BinaryTreeNode& node);
	bool RemoveNode(const BinaryTreeNode& node);
public:
	size_t GetLayerCount() const { return usefulNodes.size(); }
	bool TryAllocate(uint targetLevel, AllocatedChunkHandle& result);
	void Return(const AllocatedChunkHandle& target);
	BinaryAllocator(
		uint32_t maximumLayer);
	~BinaryAllocator();
};

inline size_t BinaryAllocator::AllocatedChunkHandleHash::operator()(const AllocatedChunkHandle& handle) const noexcept
{
	BinaryTreeNodeHash hash;
	return hash(handle.node);
}