#include "Texture.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/DescriptorHeap.h"
#include <fstream>
#include "ComputeShader.h"
#include "../Singleton/ShaderCompiler.h"
#include "RenderCommand.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/Graphics.h"
#include "TextureHeap.h"
#include "../Common/Pool.h"
#include "Utility/ITextureAllocator.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;
struct TextureFormat_LoadData
{
	DXGI_FORMAT format;
	uint pixelSize;
	bool bcCompress;
};
TextureFormat_LoadData Texture_GetFormat(TextureData::LoadFormat loadFormat)
{
	TextureFormat_LoadData loadData;
	loadData.bcCompress = false;
	switch (loadFormat)
	{
	case TextureData::LoadFormat_R8G8B8A8_UNorm:
		loadData.pixelSize = 4;
		loadData.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case TextureData::LoadFormat_R16G16B16A16_UNorm:
		loadData.pixelSize = 8;
		loadData.format = DXGI_FORMAT_R16G16B16A16_UNORM;
		break;
	case TextureData::LoadFormat_R16G16B16A16_SFloat:
		loadData.pixelSize = 8;
		loadData.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	case TextureData::LoadFormat_R32G32B32A32_SFloat:
		loadData.pixelSize = 16;
		loadData.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	case TextureData::LoadFormat_R16G16_UNorm:
		loadData.pixelSize = 4;
		loadData.format = DXGI_FORMAT_R16G16_UNORM;
		break;
	case TextureData::LoadFormat_R16G16_SFloat:
		loadData.pixelSize = 4;
		loadData.format = DXGI_FORMAT_R16G16_FLOAT;
		break;
	case TextureData::LoadFormat_BC7:
		loadData.pixelSize = 1;
		loadData.format = DXGI_FORMAT_BC7_UNORM;
		loadData.bcCompress = true;
		break;
	case TextureData::LoadFormat_BC6H:
		loadData.pixelSize = 1;
		loadData.format = DXGI_FORMAT_BC6H_UF16;
		loadData.bcCompress = true;
		break;
	case TextureData::LoadFormat_BC5U:
		loadData.pixelSize = 1;
		loadData.format = DXGI_FORMAT_BC5_UNORM;
		loadData.bcCompress = true;
		break;
	case TextureData::LoadFormat_BC5S:
		loadData.pixelSize = 1;
		loadData.format = DXGI_FORMAT_BC5_SNORM;
		loadData.bcCompress = true;
		break;
	case TextureData::LoadFormat_R32_UINT:
		loadData.pixelSize = 4;
		loadData.format = DXGI_FORMAT_R32_UINT;
		break;
	case TextureData::LoadFormat_R32G32_UINT:
		loadData.pixelSize = 8;
		loadData.format = DXGI_FORMAT_R32G32_UINT;
		break;
	case TextureData::LoadFormat_R32G32B32A32_UINT:
		loadData.pixelSize = 16;
		loadData.format = DXGI_FORMAT_R32G32B32A32_UINT;
		break;
	case TextureData::LoadFormat_R16_UNorm:
		loadData.pixelSize = 2;
		loadData.format = DXGI_FORMAT_R16_UNORM;
		break;
	case TextureData::LoadFormat_R16_UINT:
		loadData.pixelSize = 2;
		loadData.format = DXGI_FORMAT_R16_UINT;
		break;
	case TextureData::LoadFormat_R16G16_UINT:
		loadData.pixelSize = 4;
		loadData.format = DXGI_FORMAT_R16G16_UINT;
		break;
	case TextureData::LoadFormat_R16G16B16A16_UINT:
		loadData.pixelSize = 8;
		loadData.format = DXGI_FORMAT_R16G16B16A16_UINT;
		break;
	case TextureData::LoadFormat_R8_UINT:
		loadData.pixelSize = 1;
		loadData.format = DXGI_FORMAT_R8_UINT;
		break;
	case TextureData::LoadFormat_R8G8_UINT:
		loadData.pixelSize = 2;
		loadData.format = DXGI_FORMAT_R8G8_UINT;
		break;
	case TextureData::LoadFormat_R8G8B8A8_UINT:
		loadData.pixelSize = 4;
		loadData.format = DXGI_FORMAT_R8G8B8A8_UINT;
		break;
	case TextureData::LoadFormat_R32_SFloat:
		loadData.pixelSize = 4;
		loadData.format = DXGI_FORMAT_R32_FLOAT;
		break;
	}
	return loadData;
}
namespace TextureGlobal
{
	void GetData(ArrayList<char>& datas, std::ifstream& ifs, uint64 offset, uint64 size)
	{
		datas.resize(size);
		ifs.seekg(offset, std::ios::beg);
		ifs.read(datas.data(), size);
	}
	void GetData(ArrayList<char>& datas, const vengine::string& path, uint64 offset, uint64 size)
	{
		std::ifstream ifs(path.data(), std::ios::binary);
		GetData(datas, ifs, offset, size);
	}

	void ReadData(const vengine::string& str, ArrayList<char>& datas,
		TextureData& headerResult, uint startMipLevel,
		uint maximumMipLevel, uint64& targetOffset,
		uint64& targetSize, bool startLoading, bool alreadyHaveHeader)
	{
		std::ifstream ifs;
		ifs.open(str.data(), std::ios::binary);

		if (!ifs)
		{
#ifdef _DEBUG
			throw "File Not Exists!";
#else
			return;
#endif
		}
		if (alreadyHaveHeader)
			ifs.seekg(sizeof(TextureData), std::ios::beg);
		else
			TextureData::ReadTextureDataFromFile(ifs, headerResult);
		//Set Mip
		headerResult.mipCount = Max<uint>(headerResult.mipCount, 1);
		startMipLevel = Min(startMipLevel, headerResult.mipCount - 1);
		maximumMipLevel = Max<uint>(maximumMipLevel, 1);
		maximumMipLevel = Min(maximumMipLevel, headerResult.mipCount - startMipLevel);
		headerResult.mipCount = maximumMipLevel;
		UINT formt = (UINT)headerResult.format;
		if (formt >= (UINT)(TextureData::LoadFormat_Num) ||
			(UINT)headerResult.textureType >= (UINT)TextureDimension::Num)
		{
#ifdef _DEBUG
			throw "Invalide Format";
#else
			return;
#endif
		}
		UINT stride = 0;
		TextureFormat_LoadData loadData = Texture_GetFormat(headerResult.format);
		stride = loadData.pixelSize;
		if (headerResult.depth != 1 && startMipLevel != 0)
		{
#ifdef _DEBUG
			throw "Non-2D map can not use mip streaming!";
#else
			return;
#endif
		}

		uint64_t size = 0;
		uint64_t offsetSize = 0;
		UINT depth = headerResult.depth;

		for (UINT j = 0; j < depth; ++j)
		{
			UINT width = headerResult.width;
			UINT height = headerResult.height;

			for (uint i = 0; i < startMipLevel; ++i)
			{
				uint64_t currentChunkSize = 0;
				if (loadData.bcCompress)
					currentChunkSize = ((stride * (uint64_t)width * 4 + 255) & ~255) * height / 4;
				else
					currentChunkSize = ((stride * (uint64_t)width + 255) & ~255) * height;
				offsetSize += currentChunkSize;
				width /= 2;
				height /= 2;
				width = Max<uint>(1, width);
				height = Max<uint>(1, height);
			}
			headerResult.width = width;
			headerResult.height = height;
			for (UINT i = 0; i < headerResult.mipCount; ++i)
			{
				uint64_t currentChunkSize = 0;
				if (loadData.bcCompress)
					currentChunkSize = ((stride * width * 4 + 255) & ~255) * height / 4;
				else
					currentChunkSize = ((stride * (uint64_t)width + 255) & ~255) * height;
				size += currentChunkSize;
				width /= 2;
				height /= 2;
				width = Max<uint>(1, width);
				height = Max<uint>(1, height);
			}
		}
		targetOffset = offsetSize + sizeof(TextureData);
		targetSize = size;
		if (startLoading)
		{
			GetData(datas, ifs, targetOffset, targetSize);
		}
	}
}

void TextureData::ReadTextureDataFromFile(std::ifstream& ifs, TextureData& result)
{
	ifs.seekg(0, std::ios::beg);
	ifs.read((char*)&result, sizeof(TextureData));
}
class TextureLoadCommand : public RenderCommand
{
private:
	StackObject<UploadBuffer, true> containedUbuffer;
	UploadBuffer* ubuffer;
	ObjWeakPtr<ID3D12Resource*> resPtr;
	TextureData::LoadFormat loadFormat;
	UINT width;
	UINT height;
	UINT mip;
	UINT arraySize;
	TextureDimension type;
	bool* flag;
public:
	TextureLoadCommand(ID3D12Device* device,
		UINT element,
		void* dataPtr,
		const ObjectPtr<ID3D12Resource*> resPtr,
		TextureData::LoadFormat loadFormat,
		UINT width,
		UINT height,
		UINT mip,
		UINT arraySize,
		TextureDimension type, IBufferAllocator* bufferAllocator, bool* flag) :
		resPtr(resPtr), loadFormat(loadFormat), width(width), height(height), mip(mip), arraySize(arraySize), type(type),
		flag(flag)
	{
		containedUbuffer.New(device, element, false, 1, bufferAllocator);
		ubuffer = containedUbuffer;
		ubuffer->CopyDatas(0, element, dataPtr);
	}

	TextureLoadCommand(
		UploadBuffer* buffer,
		const ObjectPtr<ID3D12Resource*> resPtr,
		TextureData::LoadFormat loadFormat,
		UINT width,
		UINT height,
		UINT mip,
		UINT arraySize,
		TextureDimension type, bool* flag
	) :resPtr(resPtr), loadFormat(loadFormat),
		width(width), height(height), mip(mip), arraySize(arraySize), type(type), ubuffer(buffer),
		flag(flag)
	{

	}
	virtual void Execute(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* directCommandList,
		ID3D12GraphicsCommandList* copyCommandList,
		FrameResource* resource,
		TransitionBarrierBuffer* barrier)
	{
		if (!resPtr) return;
		auto res = *resPtr;
		
		UINT offset = 0;
		TextureFormat_LoadData loadData = Texture_GetFormat(loadFormat);
		if (type == TextureDimension::Tex3D)
		{
			UINT curWidth = width;
			UINT curHeight = height;
			for (UINT i = 0; i < mip; ++i)
			{
				Graphics::CopyBufferToTexture(
					copyCommandList,
					ubuffer,
					offset,
					res,
					i,
					curWidth, curHeight,
					arraySize,
					loadData.format, loadData.pixelSize
				);
				UINT chunkOffset = (((loadData.pixelSize * curWidth) + 255) & ~255) * curHeight;
				offset += chunkOffset;
				curWidth /= 2;
				curHeight /= 2;
			}

		}
		else
		{
			for (UINT j = 0; j < arraySize; ++j)
			{
				UINT curWidth = width;
				UINT curHeight = height;


				for (UINT i = 0; i < mip; ++i)
				{
					if (loadData.bcCompress)
					{
						Graphics::CopyBufferToBCTexture(
							copyCommandList,
							ubuffer,
							offset,
							res,
							(j * mip) + i,
							curWidth, curHeight,
							1,
							loadData.format, loadData.pixelSize
						);
						UINT chunkOffset = ((loadData.pixelSize * curWidth * 4 + 255) & ~255) * curHeight / 4;
						offset += chunkOffset;
					}
					else
					{
						Graphics::CopyBufferToTexture(
							copyCommandList,
							ubuffer,
							offset,
							res,
							(j * mip) + i,
							curWidth, curHeight,
							1,
							loadData.format, loadData.pixelSize
						);
						UINT chunkOffset = ((loadData.pixelSize * curWidth + 255) & ~255) * curHeight;
						offset += chunkOffset;
					}

					curWidth /= 2;
					curHeight /= 2;
				}
			}
		}
		uint64_t ofst = offset;
		uint64_t size = ubuffer->GetElementCount();
		*flag = true;
		ubuffer->DisposeAllocatorAfterFrame(resource);
		barrier->AddCommand(D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ, res);
	}
};

namespace TextureGlobal
{
	std::mutex mtx;
	Pool<TextureLoadCommand> cmdPool(100);
}

uint64_t Texture::GetSizeFromProperty(
	ID3D12Device* device,
	uint width,
	uint height,
	uint depth,
	TextureDimension textureType,
	uint mipCount,
	DXGI_FORMAT format)
{
	mipCount = Max<uint>(1, mipCount);
	if (textureType == TextureDimension::Cubemap)
		depth = 6;
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = textureType == TextureDimension::Tex3D ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = depth;
	texDesc.MipLevels = mipCount;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	return device->GetResourceAllocationInfo(
		0, 1, &texDesc).SizeInBytes;
}


Texture::Texture(
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
	TextureHeap* placedHeap,
	uint64_t placedOffset) : ITexture()
{
	dimension = textureType;
	fileLoadFormat = format;
	if (textureType == TextureDimension::Cubemap)
		depth = 6;
	auto loadData = Texture_GetFormat(format);
	mFormat = loadData.format;
	this->depthSlice = depth;
	this->mWidth = width;
	this->mHeight = height;
	mipCount = Max<uint>(1, mipCount);
	this->mipCount = mipCount;
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = textureType == TextureDimension::Tex3D ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = depth;
	texDesc.MipLevels = mipCount;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resourceSize = device->GetResourceAllocationInfo(
		0, 1, &texDesc).SizeInBytes;
	if (placedHeap)
	{
		ThrowIfFailed(device->CreatePlacedResource(
			placedHeap->GetHeap(),
			placedOffset,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	else
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	resTracker = ObjectPtr<ID3D12Resource*>::MakePtrNoMemoryFree(Resource.GetAddressOf());
	TextureLoadCommand cmd(
		buffer,
		resTracker,
		format,
		width,
		height,
		mipCount,
		depth,
		dimension, &loaded);
	TransitionBarrierBuffer barrierBuffer;
	cmd.Execute(device, commandList, commandList, resource, &barrierBuffer);
	barrierBuffer.ExecuteCommand(commandList);
	BindSRVToHeap(Graphics::GetGlobalDescHeap(), GetGlobalDescIndex(), device);
}

D3D12_RESOURCE_DESC Texture::CreateWithoutResource(
	TextureData& data,
	ID3D12Device* device,
	const vengine::string& filePath,
	bool startLoadNow,
	bool alreadyHaveTextureData,
	TextureDimension type,
	uint32_t maximumLoadMipmap,
	uint32_t startMipMap)
{
	dimension = type;

	targetFilePath = filePath;
	TextureGlobal::ReadData(filePath, datas, data, startMipMap, maximumLoadMipmap, fileReadOffset, fileReadSize, startLoadNow, alreadyHaveTextureData);
	if (data.textureType != type)
	{
#ifdef _DEBUG
		throw "Texture Type Not Match Exception";
#else
		return D3D12_RESOURCE_DESC();
#endif
	}

	if (type == TextureDimension::Cubemap && data.depth != 6)
	{
#ifdef _DEBUG
		throw "Cubemap's tex size must be 6";
#else
		return D3D12_RESOURCE_DESC();
#endif
	}
	auto loadData = Texture_GetFormat(data.format);
	mFormat = loadData.format;
	this->depthSlice = data.depth;
	this->mWidth = data.width;
	this->mHeight = data.height;
	this->mipCount = data.mipCount;
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = type == TextureDimension::Tex3D ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = data.width;
	texDesc.Height = data.height;
	texDesc.DepthOrArraySize = data.depth;
	texDesc.MipLevels = data.mipCount;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resourceSize = device->GetResourceAllocationInfo(
		0, 1, &texDesc).SizeInBytes;
	return texDesc;
}

void Texture::LoadTexture(ID3D12Device* device, IBufferAllocator* allocator)
{
	if (isStartLoad) return;
	isStartLoad = true;
	if (datas.empty())
	{
		TextureGlobal::GetData(datas, targetFilePath, fileReadOffset, fileReadSize);
	}
	TextureLoadCommand* cmd;
	resTracker = ObjectPtr<ID3D12Resource*>::MakePtrNoMemoryFree(Resource.GetAddressOf());
	cmd = TextureGlobal::cmdPool.New_Lock(
		TextureGlobal::mtx,
		device,
		datas.size(),
		datas.data(),
		resTracker,
		fileLoadFormat,
		mWidth,
		mHeight,
		mipCount,
		depthSlice,
		dimension, allocator, &loaded);

	RenderCommand::AddCommand(cmd, &TextureGlobal::cmdPool, &TextureGlobal::mtx);
	datas.dispose();
}

Texture::Texture(
	ID3D12Device* device,
	UINT width,
	UINT height,
	UINT depth,
	TextureDimension textureType,
	UINT mipCount,
	DXGI_FORMAT format,
	TextureHeap* placedHeap,
	uint64_t placedOffset
)
{
	dimension = textureType;
	if (textureType == TextureDimension::Cubemap)
		depth = 6;
	mFormat = format;
	this->depthSlice = depth;
	this->mWidth = width;
	this->mHeight = height;
	mipCount = Max<uint>(1, mipCount);
	this->mipCount = mipCount;
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = textureType == TextureDimension::Tex3D ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = depth;
	texDesc.MipLevels = mipCount;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resourceSize = device->GetResourceAllocationInfo(
		0, 1, &texDesc).SizeInBytes;
	if (placedHeap)
	{
		ThrowIfFailed(device->CreatePlacedResource(
			placedHeap->GetHeap(),
			placedOffset,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	else
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	BindSRVToHeap(Graphics::GetGlobalDescHeap(), GetGlobalDescIndex(), device);
}

Texture::Texture(
	ID3D12Device* device,
	const vengine::string& filePath,
	ITextureAllocator* allocator,
	IBufferAllocator* bufferAllocator,
	bool startLoading,
	TextureDimension type,
	uint32_t maximumLoadMipmap,
	uint32_t startMipMap) : allocator(allocator)
{
	ID3D12Heap* placedHeap;
	uint64_t placedOffset;
	TextureData data;
	auto texDesc = CreateWithoutResource(data, device, filePath, startLoading, false, type, maximumLoadMipmap, startMipMap);
	fileLoadFormat = data.format;
	allocator->AllocateTextureHeap(device, mFormat, mWidth, mHeight, data.depth, type, data.mipCount, &placedHeap, &placedOffset, this);
	ThrowIfFailed(device->CreatePlacedResource(
		placedHeap,
		placedOffset,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&Resource)));
	if (startLoading)
	{
		LoadTexture(device, bufferAllocator);
	}
	BindSRVToHeap(Graphics::GetGlobalDescHeap(), GetGlobalDescIndex(), device);
}

Texture::Texture(
	ID3D12Device* device,
	const vengine::string& filePath,
	bool startLoading,
	TextureDimension type,
	uint32_t maximumLoadMipmap,
	uint32_t startMipMap,
	TextureHeap* placedHeap,
	uint64_t placedOffset)
{
	TextureData data;
	auto  texDesc = CreateWithoutResource(data, device, filePath, startLoading, false, type, maximumLoadMipmap, startMipMap);
	fileLoadFormat = data.format;
	if (placedHeap)
	{
		ThrowIfFailed(device->CreatePlacedResource(
			placedHeap->GetHeap(),
			placedOffset,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	else
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	if (startLoading)
	{
		LoadTexture(device, nullptr);
	}
	BindSRVToHeap(Graphics::GetGlobalDescHeap(), GetGlobalDescIndex(), device);
}

Texture::Texture(
	ID3D12Device* device,
	const vengine::string& filePath,
	const TextureData& texData,
	bool startLoading,
	uint32_t maximumLoadMipmap,
	uint32_t startMipMap,
	TextureHeap* placedHeap,
	uint64_t placedOffset)
{
	TextureData data;
	memcpy(&data, &texData, sizeof(TextureData));
	auto  texDesc = CreateWithoutResource(data, device, filePath, startLoading, true, data.textureType, maximumLoadMipmap, startMipMap);
	fileLoadFormat = data.format;
	if (placedHeap)
	{
		ThrowIfFailed(device->CreatePlacedResource(
			placedHeap->GetHeap(),
			placedOffset,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	else
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}
	if (startLoading)
	{
		LoadTexture(device, nullptr);
	}
	BindSRVToHeap(Graphics::GetGlobalDescHeap(), GetGlobalDescIndex(), device);
}
Texture::Texture(
	ID3D12Device* device,
	const vengine::string& filePath,
	const TextureData& texData,
	ITextureAllocator* allocator,
	IBufferAllocator* bufferAllocator,
	bool startLoading,
	uint32_t maximumLoadMipmap,
	uint32_t startMipMap) : allocator(allocator)
{
	ID3D12Heap* placedHeap;
	uint64_t placedOffset;
	TextureData data;
	memcpy(&data, &texData, sizeof(TextureData));
	auto texDesc = CreateWithoutResource(data, device, filePath, startLoading, true, data.textureType, maximumLoadMipmap, startMipMap);
	fileLoadFormat = data.format;
	allocator->AllocateTextureHeap(device, mFormat, mWidth, mHeight, data.depth, data.textureType, data.mipCount, &placedHeap, &placedOffset, this);
	ThrowIfFailed(device->CreatePlacedResource(
		placedHeap,
		placedOffset,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,//D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&Resource)));
	if (startLoading)
	{
		LoadTexture(device, bufferAllocator);
	}
	BindSRVToHeap(Graphics::GetGlobalDescHeap(), GetGlobalDescIndex(), device);
}

Texture::~Texture()
{
	if (allocator)
	{
		allocator->ReturnTexture(this);
	}
}

void Texture::GetResourceViewDescriptor(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)const
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	auto format = mFormat;
	switch (dimension) {
	case TextureDimension::Tex2D:
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = mipCount;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case TextureDimension::Tex3D:
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MipLevels = mipCount;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
		break;
	case TextureDimension::Cubemap:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = mipCount;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		srvDesc.Format = format;
		break;
	}
}

void Texture::BindSRVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	GetResourceViewDescriptor(srvDesc);
	targetHeap->CreateSRV(device, this, &srvDesc, index);

}