#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/ArrayList.h"
class IGPUResource;
class DescriptorHeap : public MObject
{
public:
	DescriptorHeap(
		ID3D12Device* pDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		uint NumDescriptors,
		bool bShaderVisible = false);
	DescriptorHeap() : pDH(nullptr), recorder(nullptr)
	{
		memset(&Desc, 0, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	}
	void Create(ID3D12Device* pDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		uint NumDescriptors,
		bool bShaderVisible = false);
	ID3D12DescriptorHeap* Get() const { return pDH.Get(); }
	constexpr D3D12_GPU_DESCRIPTOR_HANDLE hGPU(uint index) const
	{
		if (index >= Desc.NumDescriptors) index = Desc.NumDescriptors - 1;
		D3D12_GPU_DESCRIPTOR_HANDLE h = { hGPUHeapStart.ptr + index * HandleIncrementSize };
		return h;
	}
	inline void SetDescriptorHeap(ID3D12GraphicsCommandList* commandList) const
	{
		ID3D12DescriptorHeap* heap = pDH.Get();
		commandList->SetDescriptorHeaps(1, &heap);
	}
	constexpr D3D12_DESCRIPTOR_HEAP_DESC GetDesc() const { return Desc; };
	constexpr uint Size() const { return Desc.NumDescriptors; }
	void CreateUAV(ID3D12Device* device, IGPUResource const* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc, uint index);
	void CreateSRV(ID3D12Device* device, IGPUResource const* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, uint index);
	void CreateRTV(ID3D12Device* device, IGPUResource const* resource, uint depthSlice, uint mipCount, const D3D12_RENDER_TARGET_VIEW_DESC* pDesc, uint index);
	void CreateDSV(ID3D12Device* device, IGPUResource const* resource, uint depthSlice, uint mipCount, const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc, uint index);
	~DescriptorHeap();
	constexpr D3D12_CPU_DESCRIPTOR_HANDLE hCPU(uint index) const
	{
		if (index >= Desc.NumDescriptors) index = Desc.NumDescriptors - 1;
		D3D12_CPU_DESCRIPTOR_HANDLE h = { hCPUHeapStart.ptr + index * HandleIncrementSize };
		return h;
	}
private:
	enum class BindType : uint
	{
		SRV = 0,
		UAV = 1,
		RTV = 2,
		DSV = 3
	};
	struct alignas(uint64) BindData
	{
		BindType type;
		uint instanceID;
		bool operator==(const BindData& data) const noexcept
		{
			return *((uint64 const*)this) == *((uint64 const*)&data);
		}
		bool operator!=(const BindData& data) const noexcept
		{
			return !operator==(data);
		}
	};
	BindData* recorder;
	std::mutex mtx;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pDH;
	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	D3D12_CPU_DESCRIPTOR_HANDLE hCPUHeapStart;
	D3D12_GPU_DESCRIPTOR_HANDLE hGPUHeapStart;
	uint HandleIncrementSize;


};