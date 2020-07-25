#pragma once
#include "IGPUResource.h"
enum class TextureDimension : uint
{
	Tex2D = 0,
	Tex3D = 1,
	Cubemap = 2,
	Tex2DArray = 3,
	Num = 4
};
class DescriptorHeap;
class ITexture : public IGPUResource
{
protected:
	uint64_t resourceSize = 0;
private:
	uint srvDescID = 0;
protected:
	DXGI_FORMAT mFormat;
	UINT depthSlice;
	UINT mWidth = 0;
	UINT mHeight = 0;
	UINT mipCount = 1;
	TextureDimension dimension;
	ITexture();
public:
	virtual ~ITexture();
	uint64_t GetResourceSize() const
	{
		return resourceSize;
	}

	TextureDimension GetDimension() const
	{
		return dimension;
	}
	DXGI_FORMAT GetFormat() const
	{
		return mFormat;
	}
	uint GetDepthSlice() const
	{
		return depthSlice;
	}
	uint GetWidth() const
	{
		return mWidth;
	}
	uint GetHeight() const
	{
		return mHeight;
	}
	uint GetMipCount() const
	{
		return mipCount;
	}
	uint GetGlobalDescIndex() const
	{
		return srvDescID;
	}
	virtual void BindSRVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device) const = 0;
};