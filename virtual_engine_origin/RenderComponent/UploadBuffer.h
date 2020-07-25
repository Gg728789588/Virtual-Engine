#pragma once

#include "IGPUResource.h"
class FrameResource;
class IBufferAllocator;
class UploadBuffer final : public IGPUResource
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, uint64_t stride, IBufferAllocator* allocator = nullptr);
	UploadBuffer() :
		mMappedData(0),
		mStride(0),
		mElementCount(0),
		mElementByteSize(0),
		mIsConstantBuffer(false),
		allocator(nullptr){}
	void Create(ID3D12Device* device, UINT elementCount, bool isConstantBuffer, uint64_t stride, IBufferAllocator* allocator = nullptr);
	~UploadBuffer();
	void DisposeAllocatorAfterFrame(FrameResource* resource);
	inline void CopyData(UINT elementIndex, const void* data) const
	{
		char* dataPos = (char*)mMappedData;
		uint64_t offset = (uint64_t)elementIndex * mElementByteSize;
		dataPos += offset;
		memcpy(dataPos, data, mStride);
	}
	inline void CopyDataInside(UINT from, UINT to) const
	{
		char* dataPos = (char*)mMappedData;
		uint64_t fromOffset = from * (uint64_t)mElementByteSize;
		uint64_t toOffset = to * (uint64_t)mElementByteSize;
		memcpy(dataPos + toOffset, dataPos + fromOffset, mStride);
	}
	inline void CopyDatas(UINT startElementIndex, UINT elementCount, const void* data) const
	{
		char* dataPos = (char*)mMappedData;
		uint64_t offset = startElementIndex * (uint64_t)mElementByteSize;
		dataPos += offset;
		memcpy(dataPos, data, (elementCount - 1) * (uint64_t)mElementByteSize + mStride);
	}
	inline void CopyFrom(UploadBuffer* otherBuffer, UINT selfStartIndex, UINT otherBufferStartIndex, UINT elementCount) const
	{
		char* otherPtr = (char*)otherBuffer->mMappedData;
		char* currPtr = (char*)mMappedData;
		otherPtr += otherBuffer->mElementByteSize * (uint64_t)otherBufferStartIndex;
		currPtr += mElementByteSize * (uint64_t)selfStartIndex;
		memcpy(currPtr, otherPtr, elementCount * (uint64_t)mElementByteSize);
	}
	inline D3D12_GPU_VIRTUAL_ADDRESS GetAddress(UINT elementCount) const
	{
		return Resource->GetGPUVirtualAddress() + GetAddressOffset(elementCount);
	}
	inline uint64_t GetAddressOffset(uint elementCount) const
	{
		return elementCount * (uint64_t)mElementByteSize;
	}
	constexpr void* GetMappedDataPtr(UINT element) const
	{
		char* dataPos = (char*)mMappedData;
		uint64_t offset = element * (uint64_t)mElementByteSize;
		dataPos += offset;
		return dataPos;
	}
	constexpr uint64_t GetStride() const { return mStride; }
	constexpr uint64_t GetAlignedStride() const { return mElementByteSize; }

	inline UINT GetElementCount() const
	{
		return mElementCount;
	}
	UploadBuffer(const UploadBuffer& another) = delete;
private:
	struct UploadCommand
	{
		UINT startIndex;
		UINT count;
	};
	uint64_t mStride;
	void* mMappedData;
	IBufferAllocator* allocator;
	UINT mElementCount;
    UINT mElementByteSize;
    bool mIsConstantBuffer;
};