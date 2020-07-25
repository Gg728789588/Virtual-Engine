#pragma once
#include "DescriptorHeap.h"
#include "IGPUResource.h"
DescriptorHeap::DescriptorHeap(
	ID3D12Device* pDevice,
	D3D12_DESCRIPTOR_HEAP_TYPE Type,
	UINT NumDescriptors,
	bool bShaderVisible)
{
	recorder = new BindData[NumDescriptors];
	memset(recorder, 0, sizeof(BindData) * NumDescriptors);
	Desc.Type = Type;
	Desc.NumDescriptors = NumDescriptors;
	Desc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	Desc.NodeMask = 0;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(&Desc,
		__uuidof(ID3D12DescriptorHeap),
		(void**)&pDH));
	hCPUHeapStart = pDH->GetCPUDescriptorHandleForHeapStart();
	hGPUHeapStart = pDH->GetGPUDescriptorHandleForHeapStart();
	HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(Desc.Type);
}

DescriptorHeap::~DescriptorHeap()
{
	delete recorder;
}

void DescriptorHeap::Create(ID3D12Device* pDevice,
	D3D12_DESCRIPTOR_HEAP_TYPE Type,
	UINT NumDescriptors,
	bool bShaderVisible)
{
	if (recorder)
	{
		delete recorder;
	}
	recorder = new BindData[NumDescriptors];
	memset(recorder, 0, sizeof(BindData) * NumDescriptors);
	Desc.Type = Type;
	Desc.NumDescriptors = NumDescriptors;
	Desc.Flags = (bShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	Desc.NodeMask = 0;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(&Desc,
		__uuidof(ID3D12DescriptorHeap),
		(void**)&pDH));
	hCPUHeapStart = pDH->GetCPUDescriptorHandleForHeapStart();
	hGPUHeapStart = pDH->GetGPUDescriptorHandleForHeapStart();
	HandleIncrementSize = pDevice->GetDescriptorHandleIncrementSize(Desc.Type);
}

void DescriptorHeap::CreateUAV(ID3D12Device* device, IGPUResource const* resource,  const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, uint index)
{
	BindData targetBindType = { BindType::UAV, resource->GetInstanceID() };
	{
		std::lock_guard<std::mutex> lck(mtx);
		if (recorder[index] == targetBindType)
			return;
		recorder[index] = targetBindType;
	}
	device->CreateUnorderedAccessView(resource->GetResource(), nullptr, pDesc, hCPU(index));
}

void DescriptorHeap::CreateSRV(ID3D12Device* device, IGPUResource const* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, uint index)
{
	BindData targetBindType = { BindType::SRV, resource->GetInstanceID() };
	{
		std::lock_guard<std::mutex> lck(mtx);
		if (recorder[index] == targetBindType)
			return;
		recorder[index] = targetBindType;
	}
	device->CreateShaderResourceView(resource->GetResource(), pDesc, hCPU(index));
}

void DescriptorHeap::CreateRTV(ID3D12Device* device, IGPUResource const* resource, uint depthSlice, uint mipCount, const D3D12_RENDER_TARGET_VIEW_DESC* pDesc, uint index)
{
	BindData targetBindType = { BindType::RTV, resource->GetInstanceID() };
	{
		std::lock_guard<std::mutex> lck(mtx);
		if (recorder[index] == targetBindType)
			return;
		recorder[index] = targetBindType;
	}
	device->CreateRenderTargetView(resource->GetResource(), pDesc, hCPU(index));
}

void DescriptorHeap::CreateDSV(ID3D12Device* device, IGPUResource const* resource, uint depthSlice, uint mipCount, const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc, uint index)
{
	BindData targetBindType = { BindType::DSV, resource->GetInstanceID() };
	{
		std::lock_guard<std::mutex> lck(mtx);
		if (recorder[index] == targetBindType)
			return;
		recorder[index] = targetBindType;
	}
	device->CreateDepthStencilView(resource->GetResource(), pDesc, hCPU(index));
}