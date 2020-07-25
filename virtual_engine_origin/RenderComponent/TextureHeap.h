#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
class TextureHeap
{
private:
	Microsoft::WRL::ComPtr<ID3D12Heap> heap;
	uint64_t chunkSize;
public:
	TextureHeap() : heap(nullptr), chunkSize(0) {}
	uint64_t GetChunkSize() const { return chunkSize; }
	TextureHeap(ID3D12Device* device, uint64_t chunkSize, bool isRenderTexture);
	void Create(ID3D12Device* device, uint64_t chunkSize, bool isRenderTexture);
	ID3D12Heap* GetHeap() const
	{
		return heap.Get();
	}
};