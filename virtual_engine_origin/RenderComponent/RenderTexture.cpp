#include "RenderTexture.h"
#include "../Singleton/Graphics.h"
#include "TextureHeap.h"

void RenderTexture::ClearRenderTarget(ID3D12GraphicsCommandList* commandList, UINT slice, UINT mip)const
{
#ifndef NDEBUG
	if (dimension == TextureDimension::Tex3D)
	{
		throw "Tex3D can not be cleaned!";
	}
#endif
	if (usage == RenderTextureUsage::ColorBuffer)
	{
		float colors[4] = { clearColor, clearColor, clearColor, clearColor };
		commandList->ClearRenderTargetView(rtvHeap.hCPU(slice * mipCount + mip), colors, 0, nullptr);
	}
	else
		commandList->ClearDepthStencilView(rtvHeap.hCPU(slice * mipCount + mip), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, clearColor, 0, 0, nullptr);
}

void RenderTexture::SetViewport(ID3D12GraphicsCommandList* commandList, uint mipCount)const
{
	uint currentWidth = mWidth >> mipCount;
	uint currentHeight = mHeight >> mipCount;
	D3D12_VIEWPORT Viewport({ 0.0f, 0.0f, (float)(currentWidth), (float)(currentHeight), 0.0f, 1.0f });
	D3D12_RECT ScissorRect({ 0, 0, (LONG)currentWidth, (LONG)currentHeight });
	commandList->RSSetViewports(1, &Viewport);
	commandList->RSSetScissorRects(1, &ScissorRect);
}

void RenderTexture::GetColorUAVDesc(D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, UINT targetMipLevel)const
{
	UINT maxLevel = Resource->GetDesc().MipLevels - 1;
	targetMipLevel = Min(targetMipLevel, maxLevel);
	uavDesc.Format = Resource->GetDesc().Format;
	switch (dimension)
	{
	case TextureDimension::Tex2D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = targetMipLevel;
		uavDesc.Texture2D.PlaneSlice = 0;
		break;
	case TextureDimension::Tex3D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.MipSlice = targetMipLevel;
		uavDesc.Texture3D.WSize = depthSlice >> targetMipLevel;
		break;
	default:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.ArraySize = depthSlice;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.MipSlice = targetMipLevel;
		uavDesc.Texture2DArray.PlaneSlice = 0;
		break;
	}
}

void RenderTexture::GetColorViewDesc(D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)const
{

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	auto format = Resource->GetDesc();
	switch (format.Format)
	{
	case DXGI_FORMAT_D16_UNORM:
		srvDesc.Format = DXGI_FORMAT_R16_UNORM;
		break;
	case DXGI_FORMAT_D32_FLOAT:
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		break;
	default:
		srvDesc.Format = format.Format;
	}
	switch (dimension)
	{
	case TextureDimension::Cubemap:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = format.MipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case TextureDimension::Tex2D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = format.MipLevels;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case TextureDimension::Tex2DArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = format.MipLevels;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2DArray.ArraySize = depthSlice;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		break;
	case TextureDimension::Tex3D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MipLevels = format.MipLevels;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
		break;
	}

}
D3D12_CPU_DESCRIPTOR_HANDLE RenderTexture::GetColorDescriptor(UINT slice, UINT mip)const
{
#ifndef NDEBUG
	if (dimension == TextureDimension::Tex3D)
	{
		throw "Tex3D have no rtv heap!";
	}
#endif
	return rtvHeap.hCPU(slice * mipCount + mip);
}

uint64_t RenderTexture::GetSizeFromProperty(
	ID3D12Device* device,
	UINT width,
	UINT height,
	RenderTextureFormat rtFormat,
	TextureDimension type,
	UINT depthCount,
	UINT mipCount,
	RenderTextureState initState)
{
	mipCount = Max<uint>(mipCount, 1);
	UINT arraySize;
	switch (type)
	{
	case TextureDimension::Cubemap:
		arraySize = 6;
		break;
	case TextureDimension::Tex2D:
		arraySize = 1;
		break;
	default:
		arraySize = Max<uint>(1, depthCount);
		break;
	}
	if (rtFormat.usage == RenderTextureUsage::ColorBuffer)
	{
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = (type == TextureDimension::Tex3D) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.DepthOrArraySize = arraySize;
		texDesc.MipLevels = mipCount;
		texDesc.Format = rtFormat.colorFormat;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		return device->GetResourceAllocationInfo(
			0, 1, &texDesc).SizeInBytes;
	}
	else
	{
		DXGI_FORMAT mDepthFormat;
		DXGI_FORMAT mFormat;
		switch (rtFormat.depthFormat)
		{
		case RenderTextureDepthSettings_Depth32:
			mFormat = DXGI_FORMAT_D32_FLOAT;
			mDepthFormat = DXGI_FORMAT_D32_FLOAT;
			break;
		case RenderTextureDepthSettings_Depth16:
			mFormat = DXGI_FORMAT_D16_UNORM;
			mDepthFormat = DXGI_FORMAT_D16_UNORM;
			break;
		case RenderTextureDepthSettings_DepthStencil:
			mFormat = DXGI_FORMAT_R24G8_TYPELESS;
			mDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			break;
		default:
			mFormat = DXGI_FORMAT_UNKNOWN;
			mDepthFormat = DXGI_FORMAT_UNKNOWN;
			break;
		}

		if (mFormat != DXGI_FORMAT_UNKNOWN)
		{
			D3D12_RESOURCE_DESC depthStencilDesc;
			depthStencilDesc.Dimension = (type == TextureDimension::Tex3D) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			depthStencilDesc.Alignment = 0;
			depthStencilDesc.Width = width;
			depthStencilDesc.Height = height;
			depthStencilDesc.DepthOrArraySize = arraySize;
			depthStencilDesc.MipLevels = 1;
			depthStencilDesc.Format = mFormat;
			mFormat = mDepthFormat;
			depthStencilDesc.SampleDesc.Count = 1;
			depthStencilDesc.SampleDesc.Quality = 0;
			depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			return device->GetResourceAllocationInfo(
				0, 1, &depthStencilDesc).SizeInBytes;
		}
	}
	return 0;
}

void RenderTexture::SetName(std::wstring const& str)
{
	Resource->SetName(str.c_str());
}
RenderTexture::RenderTexture(
	ID3D12Device* device,
	UINT width,
	UINT height,
	RenderTextureFormat rtFormat,
	TextureDimension type,
	UINT depthCount,
	UINT mipCount,
	RenderTextureState initState,
	TextureHeap* targetHeap,
	uint64_t placedOffset,
	float clearColor
) : ITexture(),
usage(rtFormat.usage),
clearColor(clearColor)
{
	
	mipCount = Max<uint>(mipCount, 1);
	dimension = type;
	mWidth = width;
	mHeight = height;
	UINT arraySize;
	switch (type)
	{
	case TextureDimension::Cubemap:
		arraySize = 6;
		break;
	case TextureDimension::Tex2D:
		arraySize = 1;
		break;
	default:
		arraySize = Max<uint>(1, depthCount);
		break;
	}
	if (rtFormat.usage == RenderTextureUsage::ColorBuffer)
	{
		mFormat = rtFormat.colorFormat;


		this->mipCount = mipCount;
		depthSlice = arraySize;
		D3D12_RESOURCE_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
		texDesc.Dimension = (type == TextureDimension::Tex3D) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Alignment = 0;
		texDesc.Width = mWidth;
		texDesc.Height = mHeight;
		texDesc.DepthOrArraySize = arraySize;
		texDesc.MipLevels = mipCount;
		texDesc.Format = mFormat;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = mFormat;
		clearValue.Color[0] = clearColor;
		clearValue.Color[1] = clearColor;
		clearValue.Color[2] = clearColor;
		clearValue.Color[3] = clearColor;
		switch (initState)
		{
		case RenderTextureState::Unordered_Access:
			this->initState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			break;
		case RenderTextureState::Render_Target:
			this->initState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			break;
		case RenderTextureState::Common:
			this->initState = D3D12_RESOURCE_STATE_COMMON;
			break;
		case RenderTextureState::Generic_Read:
			this->initState = D3D12_RESOURCE_STATE_GENERIC_READ;
			break;
		default:
			if (type == TextureDimension::Tex3D)
				this->initState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			else this->initState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			break;
		}
		resourceSize = device->GetResourceAllocationInfo(
			0, 1, &texDesc).SizeInBytes;
		if (!targetHeap)
		{
			ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				this->initState,
				&clearValue,
				IID_PPV_ARGS(&Resource)));
		}
		else
		{
			ThrowIfFailed(device->CreatePlacedResource(
				targetHeap->GetHeap(),
				placedOffset,
				&texDesc,
				this->initState,
				&clearValue,
				IID_PPV_ARGS(&Resource)));
		}
		if (type != TextureDimension::Tex3D)
			rtvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, arraySize * mipCount, false);
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		switch (type)
		{
		case TextureDimension::Tex2D:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = mFormat;
			rtvDesc.Texture2D.PlaneSlice = 0;
			for (uint i = 0; i < mipCount; ++i)
			{
				rtvDesc.Texture2D.MipSlice = i;
				rtvHeap.CreateRTV(device, this, 0, i, &rtvDesc, i);
			}
			break;

		case TextureDimension::Tex3D:
			break;
		default:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Format = mFormat;


			rtvDesc.Texture2DArray.PlaneSlice = 0;
			rtvDesc.Texture2DArray.ArraySize = 1;
			for (int i = 0, count = 0; i < arraySize; ++i)
			{
				for (uint j = 0; j < mipCount; ++j)
				{
					// Render target to ith element.
					rtvDesc.Texture2DArray.FirstArraySlice = i;
					rtvDesc.Texture2DArray.MipSlice = j;
					// Create RTV to ith cubemap face.
					rtvHeap.CreateRTV(device, this, i, j, &rtvDesc, count);
					count++;
				}
			}
			break;
		}
		uavDescIndices.resize(mipCount);
		for (uint i = 0; i < mipCount; ++i)
		{
			uavDescIndices[i] = Graphics::GetDescHeapIndexFromPool();
			BindUAVToHeap(Graphics::GetGlobalDescHeap(), uavDescIndices[i], device, i);
		}
	}
	else
	{
		DXGI_FORMAT mDepthFormat;
		switch (rtFormat.depthFormat)
		{
		case RenderTextureDepthSettings_Depth32:
			mFormat = DXGI_FORMAT_D32_FLOAT;
			mDepthFormat = DXGI_FORMAT_D32_FLOAT;
			break;
		case RenderTextureDepthSettings_Depth16:
			mFormat = DXGI_FORMAT_D16_UNORM;
			mDepthFormat = DXGI_FORMAT_D16_UNORM;
			break;
		case RenderTextureDepthSettings_DepthStencil:
			mFormat = DXGI_FORMAT_R24G8_TYPELESS;
			mDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			break;
		default:
			mFormat = DXGI_FORMAT_UNKNOWN;
			mDepthFormat = DXGI_FORMAT_UNKNOWN;
			break;
		}

		if (mFormat != DXGI_FORMAT_UNKNOWN)
		{
			rtvHeap.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, arraySize * mipCount, false);
			D3D12_RESOURCE_DESC depthStencilDesc;
			depthStencilDesc.Dimension = (type == TextureDimension::Tex3D) ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			depthStencilDesc.Alignment = 0;
			depthStencilDesc.Width = width;
			depthStencilDesc.Height = height;
			depthStencilDesc.DepthOrArraySize = arraySize;
			depthStencilDesc.MipLevels = 1;
			depthStencilDesc.Format = mFormat;
			mFormat = mDepthFormat;

			depthStencilDesc.SampleDesc.Count = 1;
			depthStencilDesc.SampleDesc.Quality = 0;
			depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			D3D12_CLEAR_VALUE depthClearValue;
			depthClearValue.Format = mFormat;
			depthClearValue.DepthStencil.Depth = clearColor;
			depthClearValue.DepthStencil.Stencil = 0;

			switch (initState)
			{
			case RenderTextureState::Unordered_Access:
				this->initState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
				break;
			case RenderTextureState::Generic_Read:
				this->initState = D3D12_RESOURCE_STATE_DEPTH_READ;
				break;
			case RenderTextureState::Common:
				this->initState = D3D12_RESOURCE_STATE_COMMON;
				break;
			default:
				this->initState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
				break;
			}
			resourceSize = device->GetResourceAllocationInfo(
				0, 1, &depthStencilDesc).SizeInBytes;
			if (!targetHeap)
			{
				ThrowIfFailed(device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&depthStencilDesc,
					this->initState,
					&depthClearValue,
					IID_PPV_ARGS(&Resource)));
			}
			else
			{
				ThrowIfFailed(device->CreatePlacedResource(
					targetHeap->GetHeap(),
					placedOffset,
					&depthStencilDesc,
					this->initState,
					&depthClearValue,
					IID_PPV_ARGS(&Resource)));
			}
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
			switch (type)
			{
			case TextureDimension::Tex2D:
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Format = mDepthFormat;
				for (uint i = 0; i < mipCount; ++i)
				{
					dsvDesc.Texture2D.MipSlice = i;
					rtvHeap.CreateDSV(device, this, 0, i, &dsvDesc, i);
				}
				break;
			default:
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;

				dsvDesc.Texture2DArray.ArraySize = 1;

				for (int i = 0, count = 0; i < arraySize; ++i)
				{
					for (uint j = 0; j < mipCount; ++j)
					{
						dsvDesc.Texture2DArray.MipSlice = j;
						dsvDesc.Texture2DArray.FirstArraySlice = i;
						rtvHeap.CreateDSV(device, this, i, j, &dsvDesc, count);
						count++;
					}
				}
				break;

			}

		}

	}
	BindSRVToHeap(Graphics::GetGlobalDescHeap(), GetGlobalDescIndex(), device);
	if (usage == RenderTextureUsage::DepthBuffer)
		writeState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	else writeState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	readState = (bool)usage ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_GENERIC_READ;
}

void RenderTexture::BindRTVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT slice, UINT mip)const
{
	if (usage == RenderTextureUsage::ColorBuffer)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		switch (dimension)
		{
		case TextureDimension::Tex2D:
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = mFormat;
			rtvDesc.Texture2D.MipSlice = mip;
			rtvDesc.Texture2D.PlaneSlice = 0;
			targetHeap->CreateRTV(device, this, 0, mip, &rtvDesc, index * mipCount + mip);
			break;

		case TextureDimension::Tex3D:
			break;
		default:

			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Format = mFormat;
			rtvDesc.Texture2DArray.MipSlice = mip;
			rtvDesc.Texture2DArray.PlaneSlice = 0;
			rtvDesc.Texture2DArray.ArraySize = 1;
			rtvDesc.Texture2DArray.FirstArraySlice = slice;
			targetHeap->CreateRTV(device, this, slice, mip, &rtvDesc, index * mipCount + mip);
			break;
		}
	}
	else
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		switch (dimension)
		{
		case TextureDimension::Tex2D:
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Format = mFormat;
			dsvDesc.Texture2D.MipSlice = 0;
			break;
		default:
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;

			dsvDesc.Texture2DArray.ArraySize = 1;
			dsvDesc.Texture2DArray.MipSlice = 0;
			dsvDesc.Texture2DArray.FirstArraySlice = slice;
			break;

		}
		targetHeap->CreateDSV(device, this, 0, 0, &dsvDesc, index);
	}
}

void RenderTexture::BindSRVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device)const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	GetColorViewDesc(srvDesc);
	targetHeap->CreateSRV(device, this, &srvDesc, index);
}

void RenderTexture::BindUAVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device, UINT targetMipLevel)const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	GetColorUAVDesc(uavDesc, targetMipLevel);
	targetHeap->CreateUAV(device, this, &uavDesc, index);
}

RenderTexture::~RenderTexture()
{
	for (auto ite = uavDescIndices.begin(); ite != uavDescIndices.end(); ++ite)
	{
		Graphics::ReturnDescHeapIndexToPool(*ite);
	}
}

uint RenderTexture::GetGlobalUAVDescIndex(uint mipLevel) const
{
	if (usage == RenderTextureUsage::ColorBuffer)
	{
		mipLevel = Min<uint>(mipLevel, mipCount - 1);
		return uavDescIndices[mipLevel];
	}
	else
	{
#if defined(DEBUG) || defined(_DEBUG)
		throw "Depth UAV Not Allowed!";
#else
		return 0;
#endif
	}
}