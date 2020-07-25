#include "ReadbackBuffer.h"
#include "../Singleton/FrameResource.h"
ReadbackBuffer::ReadbackBuffer(ID3D12Device* device, UINT elementCount, size_t stride)
{
	mElementCount = elementCount;
	mStride = stride;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(stride * elementCount),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&Resource)));
//	ThrowIfFailed(Resource->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

	// We do not need to unmap until we are done with the resource.  However, we must not write to
	// the resource while it is in use by the GPU (so we must use synchronization techniques).
}

void ReadbackBuffer::Create(ID3D12Device* device, UINT elementCount, size_t stride)
{

	mElementCount = elementCount;
	mStride = stride;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(stride * elementCount),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&Resource)));
}

void ReadbackBuffer::Map()
{
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = mElementCount * mStride;
	ThrowIfFailed(Resource->Map(0, &range, reinterpret_cast<void**>(&mMappedData)));
}

void ReadbackBuffer::UnMap()
{
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = mElementCount * mStride;
	Resource->Unmap(0, nullptr);
	mMappedData = nullptr;
}