#pragma once
#include "ITextureAllocator.h"
namespace D3D12MA
{
	class Allocator;
	class Allocation;
}
class DefaultTextureAllocator final : public ITextureAllocator
{
private:
	HashMap<uint, D3D12MA::Allocation*> allocatedTexs;
	D3D12MA::Allocator* allocator = nullptr;
	D3D12_HEAP_FLAGS heapFlag;
	bool isRenderTexture;
public:
	~DefaultTextureAllocator();
	virtual void AllocateTextureHeap(
		ID3D12Device* device,
		DXGI_FORMAT format,
		uint32_t width,
		uint32_t height,
		uint32_t depthSlice,
		TextureDimension dimension,
		uint32_t mipCount,
		ID3D12Heap** heap, uint64_t* offset,
		ITexture* currentPtr);
	virtual void ReturnTexture(ITexture* tex);
	DefaultTextureAllocator(
		ID3D12Device* device,
		IDXGIAdapter* adapter,
		bool isRenderTexture);
};