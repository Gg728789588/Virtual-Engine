#include "BinaryAllocator.h"
#include <algorithm>
#include <stdint.h>

void BinaryAllocator::AddNode(const BinaryTreeNode& node)
{
	singleNodeCacheDict.Insert(node, { node.layer, (uint)usefulNodes[node.layer].size() });
	usefulNodes[node.layer].push_back(node);
}

bool BinaryAllocator::RemoveNode(const BinaryTreeNode& node)
{
	auto ite = singleNodeCacheDict.Find(node);
	if (!ite) return false;
	auto& vec = usefulNodes[ite.Value().layer];
	if (ite.Value().index != vec.size() - 1)
	{
		auto lastIte = vec.end() - 1;
		vec[ite.Value().index] = *lastIte;
		singleNodeCacheDict[*lastIte].index = ite.Value().index;
	}
	singleNodeCacheDict.Remove(ite);
	vec.erase(vec.end() - 1);
	return true;
}
BinaryAllocator::BinaryAllocator(
	uint32_t maximumLayer) :
	usefulNodes(maximumLayer),
	singleNodeCacheDict(23)
{
	rootChunkSize = ((uint64)1) << (maximumLayer - 1);
	BinaryTreeNode root = BinaryTreeNode(rootChunkSize, 0, 0);
	AddNode(root);
}

bool BinaryAllocator::TryAllocate(uint targetLevel, AllocatedChunkHandle& result)
{
	int32_t startLayer = targetLevel;
	while (true)
	{
		auto& currVec = usefulNodes[startLayer];
		if (currVec.empty())
		{
			if (startLayer == 0) return false;
			auto& lastVec = usefulNodes[startLayer - 1];
			if (!lastVec.empty())
			{
				auto lastIte = lastVec.end() - 1;
				uint64 subSize = lastIte->size / 2;
				BinaryTreeNode left = BinaryTreeNode(subSize, lastIte->offset, startLayer);
				BinaryTreeNode right = BinaryTreeNode(subSize, lastIte->offset + subSize, startLayer);
				AddNode(left);
				AddNode(right);
				RemoveNode(*lastIte);
				break;
			}
			startLayer--;
		}
		else
			break;
	}
	for (; startLayer < targetLevel; startLayer++)
	{
		auto lastIte = usefulNodes[startLayer].end() - 1;
		uint64 subSize = lastIte->size / 2;
		BinaryTreeNode left = BinaryTreeNode(subSize, lastIte->offset, startLayer + 1);
		BinaryTreeNode right = BinaryTreeNode(subSize, lastIte->offset + subSize, startLayer + 1);
		AddNode(left);
		AddNode(right);
		RemoveNode(*lastIte);
	}
	auto lastIte = usefulNodes[targetLevel].end() - 1;
	result.node = *lastIte;
	RemoveNode(*lastIte);
	return true;
}

BinaryAllocator::~BinaryAllocator()
{

}

void BinaryAllocator::ReturnChunk(const BinaryTreeNode& originNode)
{
	BinaryTreeNode node = originNode;
	while (true)
	{
		BinaryTreeNode buddyTree = node;
		if (!node.isRight())
		{
			buddyTree.offset += node.size;
		}
		else
		{
			buddyTree.offset -= node.size;
		}
		if (RemoveNode(buddyTree))
		{
			BinaryTreeNode fatherNode = buddyTree.isRight() ? node : buddyTree;
			fatherNode.layer--;
			fatherNode.size *= 2;
			node = fatherNode;

		}
		else
			break;
	}
	AddNode(node);
}

void BinaryAllocator::Return(const AllocatedChunkHandle& target)
{
	ReturnChunk(target.node);
}