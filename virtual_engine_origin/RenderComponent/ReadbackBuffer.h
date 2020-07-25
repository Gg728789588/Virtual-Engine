#pragma once

#include "IGPUResource.h"
class FrameResource;
class ReadbackBuffer final : public IGPUResource
{
public:
	ReadbackBuffer(ID3D12Device* device, UINT elementCount, size_t stride);
	ReadbackBuffer() :
		mMappedData(0),
		mStride(0),
		mElementCount(0) {}
	void Create(ID3D12Device* device, UINT elementCount, size_t stride);
	virtual ~ReadbackBuffer()
	{
		if (Resource != nullptr && mMappedData != nullptr)
			Resource->Unmap(0, nullptr);
	}

	inline D3D12_GPU_VIRTUAL_ADDRESS GetAddress(UINT elementCount) const
	{
		return Resource->GetGPUVirtualAddress() + elementCount * mStride;
	}
	constexpr size_t GetStride() const { return mStride; }

	inline UINT GetElementCount() const
	{
		return mElementCount;
	}
	inline void* GetMappedPtr(uint index) const
	{
		size_t sz = (size_t)mMappedData;
		return (void*)(sz + mStride * index);
	}
	ReadbackBuffer(const ReadbackBuffer& another) = delete;
	void Map();
	void UnMap();
private:
	void* mMappedData = nullptr;
	size_t mStride;
	UINT mElementCount;

};