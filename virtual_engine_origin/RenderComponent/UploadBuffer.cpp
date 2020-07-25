#include "UploadBuffer.h"
#include "../Singleton/FrameResource.h"
#include "Utility/IBufferAllocator.h"
UploadBuffer::UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, uint64_t stride, IBufferAllocator* allocator) : allocator(allocator)
{
	mIsConstantBuffer = isConstantBuffer;
	// Constant buffer elements need to be multiples of 256 bytes.
	// This is because the hardware can only view constant data 
	// at m*256 byte offsets and of n*256 byte lengths. 
	// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
	// UINT64 OffsetInBytes; // multiple of 256
	// UINT   SizeInBytes;   // multiple of 256
	// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
	mElementCount = elementCount;
	if (isConstantBuffer)
		mElementByteSize = d3dUtil::CalcConstantBufferByteSize(stride);
	else mElementByteSize = stride;
	mStride = stride;
	if (Resource)
	{
		Resource->Unmap(0, nullptr);
		Resource = nullptr;
	}
	uint64 size = (uint64)mElementByteSize * (uint64)elementCount;
	if (allocator)
	{

		ID3D12Heap* heap;
		uint64 offset;
		allocator->AllocateTextureHeap(
			device, size, D3D12_HEAP_TYPE_UPLOAD, &heap, &offset, GetInstanceID());
		ThrowIfFailed(device->CreatePlacedResource(
			heap, offset,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	else
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	ThrowIfFailed(Resource->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

	// We do not need to unmap until we are done with the resource.  However, we must not write to
	// the resource while it is in use by the GPU (so we must use synchronization techniques).
}

void UploadBuffer::Create(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, uint64_t stride, IBufferAllocator* allocator)
{
	if (this->allocator)
	{
		this->allocator->ReturnBuffer(GetInstanceID());
	}
	this->allocator = allocator;
	mIsConstantBuffer = isConstantBuffer;
	// Constant buffer elements need to be multiples of 256 bytes.
	// This is because the hardware can only view constant data 
	// at m*256 byte offsets and of n*256 byte lengths. 
	// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
	// UINT64 OffsetInBytes; // multiple of 256
	// UINT   SizeInBytes;   // multiple of 256
	// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
	mElementCount = elementCount;
	if (isConstantBuffer)
		mElementByteSize = d3dUtil::CalcConstantBufferByteSize(stride);
	else mElementByteSize = stride;
	mStride = stride;
	if (Resource)
	{
		Resource->Unmap(0, nullptr);
		Resource = nullptr;
	}
	uint64 size = (uint64)mElementByteSize * (uint64)elementCount;
	if (allocator)
	{

		ID3D12Heap* heap;
		uint64 offset;
		allocator->AllocateTextureHeap(
			device, size, D3D12_HEAP_TYPE_UPLOAD, &heap, &offset, GetInstanceID());
		ThrowIfFailed(device->CreatePlacedResource(
			heap, offset,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	else
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	ThrowIfFailed(Resource->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
}

void UploadBuffer::DisposeAllocatorAfterFrame(FrameResource* resource)
{
	if (allocator)
	{
		uint instanceID = GetInstanceID();
		auto alloc = allocator;
		resource->DisposeTargetAfterFlush([=]()->void
			{
				alloc->ReturnBuffer(instanceID);
			});
		allocator = nullptr;
	}
	if (Resource != nullptr)
	{
		Resource->Unmap(0, nullptr);
	}
	ReleaseAfterFrame(resource);
	Resource = nullptr;
}

UploadBuffer::~UploadBuffer()
{
	if (allocator)
	{
		allocator->ReturnBuffer(GetInstanceID());
	}
	if (Resource != nullptr)
		Resource->Unmap(0, nullptr);
}