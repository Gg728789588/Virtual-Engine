#pragma once
#include "../Common/MObject.h"
#include "../Common/ArrayList.h"
#include "ITexture.h"
class DescriptorHeap;
class UploadBuffer;
class FrameResource;
class TextureHeap;
class ITextureAllocator;
class IBufferAllocator;

struct TextureData
{
	UINT width;
	UINT height;
	UINT depth;
	TextureDimension textureType;
	UINT mipCount;
	enum LoadFormat
	{
		LoadFormat_R8G8B8A8_UNorm = 0,
		LoadFormat_R16G16B16A16_UNorm = 1,
		LoadFormat_R16G16B16A16_SFloat = 2,
		LoadFormat_R32G32B32A32_SFloat = 3,
		LoadFormat_R16G16_SFloat = 4,
		LoadFormat_R16G16_UNorm = 5,
		LoadFormat_BC7 = 6,
		LoadFormat_BC6H = 7,
		LoadFormat_R32_UINT = 8,
		LoadFormat_R32G32_UINT = 9,
		LoadFormat_R32G32B32A32_UINT = 10,
		LoadFormat_R16_UNorm = 11,
		LoadFormat_BC5U = 12,
		LoadFormat_BC5S = 13,
		LoadFormat_R16_UINT = 14,
		LoadFormat_R16G16_UINT = 15,
		LoadFormat_R16G16B16A16_UINT = 16,
		LoadFormat_R8_UINT = 17,
		LoadFormat_R8G8_UINT = 18,
		LoadFormat_R8G8B8A8_UINT = 19,
		LoadFormat_R32_SFloat = 20,
		LoadFormat_Num = 21
	};
	LoadFormat format;
	static void ReadTextureDataFromFile(std::ifstream& ifs,TextureData& result);
};

class Texture final : public ITexture
{
private:
	void GetResourceViewDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC& desc) const;
	ITextureAllocator* allocator = nullptr;
	bool loaded = false;
	//Loading Parameters
	TextureData::LoadFormat fileLoadFormat;
	bool isStartLoad = false;
	uint64 fileReadOffset = 0;
	uint64 fileReadSize = 0;
	ArrayList<char> datas;
	ObjectPtr<ID3D12Resource*> resTracker;
	vengine::string targetFilePath;
	Texture() {}
	D3D12_RESOURCE_DESC CreateWithoutResource(
		TextureData& data,
		ID3D12Device* device,
		const vengine::string& filePath,
		bool startLoadNow,
		bool alreadyHaveTextureData,
		TextureDimension type = TextureDimension::Tex2D,
		uint32_t maximumLoadMipmap = -1,
		uint32_t startMipMap = 0
	);

public:

	//Async Load
	Texture(
		ID3D12Device* device,
		const vengine::string& filePath,
		bool startLoading = true,
		TextureDimension type = TextureDimension::Tex2D,
		uint32_t maximumLoadMipmap = -1,
		uint32_t startMipMap = 0,
		TextureHeap* placedHeap = nullptr,
		uint64_t placedOffset = 0
	);
	Texture(
		ID3D12Device* device,
		const vengine::string& filePath,
		ITextureAllocator* allocator,
		IBufferAllocator* bufferAllocator = nullptr,
		bool startLoading = true,
		TextureDimension type = TextureDimension::Tex2D,
		uint32_t maximumLoadMipmap = -1,
		uint32_t startMipMap = 0
	);

	Texture(
		ID3D12Device* device,
		const vengine::string& filePath,
		const TextureData& texData,
		bool startLoading = true,
		uint32_t maximumLoadMipmap = -1,
		uint32_t startMipMap = 0,
		TextureHeap* placedHeap = nullptr,
		uint64_t placedOffset = 0
	);
	Texture(
		ID3D12Device* device,
		const vengine::string& filePath,
		const TextureData& texData,
		ITextureAllocator* allocator,
		IBufferAllocator* bufferAllocator = nullptr,
		bool startLoading = true,
		uint32_t maximumLoadMipmap = -1,
		uint32_t startMipMap = 0
	);

	Texture(
		ID3D12Device* device,
		UINT width,
		UINT height,
		UINT depth,
		TextureDimension textureType,
		UINT mipCount,
		DXGI_FORMAT format,
		TextureHeap* placedHeap = nullptr,
		uint64_t placedOffset = 0
	);

	//Sync Copy
	Texture(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource,
		UploadBuffer* buffer,
		UINT width,
		UINT height,
		UINT depth,
		TextureDimension textureType,
		UINT mipCount,
		TextureData::LoadFormat format,
		TextureHeap* placedHeap = nullptr,
		uint64_t placedOffset = 0
	);
	~Texture();
	static uint64_t GetSizeFromProperty(
		ID3D12Device* device,
		uint width,
		uint height,
		uint depth,
		TextureDimension textureType,
		uint mipCount,
		DXGI_FORMAT format);
	void LoadTexture(ID3D12Device* device, IBufferAllocator* allocator);
	bool IsLoaded() const { return loaded; }
	bool IsStartLoad() const { return isStartLoad; }
	virtual void BindSRVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device) const;
};
