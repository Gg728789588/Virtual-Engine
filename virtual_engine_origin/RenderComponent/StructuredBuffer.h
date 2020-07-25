#pragma once
#include "IGPUResource.h"
class FrameResource;
class DescriptorHeap;
struct StructuredBufferElement
{
	uint64_t stride;
	uint64_t elementCount;
	static StructuredBufferElement Get(uint64_t stride, uint64_t elementCount)
	{
		return { stride, elementCount };
	}
};

enum StructuredBufferState
{
	StructuredBufferState_UAV = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
	StructuredBufferState_Indirect = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
	StructuredBufferState_Read = D3D12_RESOURCE_STATE_GENERIC_READ
};

class StructuredBuffer final : public IGPUResource
{
private:
	ArrayList<StructuredBufferElement> elements;
	ArrayList<uint64_t> offsets;
	D3D12_RESOURCE_STATES initState;
	uint srvDescIndex = 0;
	uint uavDescIndex = 0;
	void InitGlobalHeap(ID3D12Device* device);
	void DisposeGlobalHeap() const;
public:
	StructuredBuffer(StructuredBuffer const&) = delete;
	StructuredBuffer(StructuredBuffer&&) = delete;
	StructuredBuffer(
		ID3D12Device* device,
		StructuredBufferElement* elementsArray,
		UINT elementsCount,
		bool isIndirect = false,
		bool isReadable = false
	);
	StructuredBuffer(
		ID3D12Device* device,
		const std::initializer_list<StructuredBufferElement>& elementsArray,
		bool isIndirect = false,
		bool isReadable = false
	);
	StructuredBuffer(
		ID3D12Device* device,
		StructuredBufferElement* elementsArray,
		UINT elementsCount,
		D3D12_RESOURCE_STATES targetState
	);
	StructuredBuffer(
		ID3D12Device* device,
		const std::initializer_list<StructuredBufferElement>& elementsArray,
		D3D12_RESOURCE_STATES targetState
	);
	uint GetSRVDescIndex() const { return srvDescIndex; }
	uint GetUAVDescIndex() const { return uavDescIndex; }
	~StructuredBuffer();
	void BindSRVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device) const;
	void BindUAVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device)const;
	D3D12_RESOURCE_STATES GetInitState() const { return initState; }
	void UploadData(ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* res,
		uint element, uint startIndex,
		uint endIndex, void const* sourceData) const;
	uint64_t GetStride(UINT index) const;
	uint64_t GetElementCount(UINT index) const;
	D3D12_GPU_VIRTUAL_ADDRESS GetAddress(UINT element, UINT index) const;
	uint64_t GetAddressOffset(UINT element, UINT index) const;
	void TransformBufferState(ID3D12GraphicsCommandList* commandList, StructuredBufferState beforeState, StructuredBufferState afterState) const;
};
/*
class StructuredBufferStateBarrier final
{
	StructuredBuffer* sbuffer;
	D3D12_RESOURCE_STATES beforeState;
	D3D12_RESOURCE_STATES afterState;
	ID3D12GraphicsCommandList* commandList;
public:
	StructuredBufferStateBarrier(ID3D12GraphicsCommandList* commandList, StructuredBuffer* sbuffer, StructuredBufferState beforeState, StructuredBufferState afterState) :
		beforeState((D3D12_RESOURCE_STATES)beforeState), afterState((D3D12_RESOURCE_STATES)afterState), sbuffer(sbuffer),
		commandList(commandList)
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			sbuffer->GetResource(),
			this->beforeState,
			this->afterState
		));
	}
	~StructuredBufferStateBarrier()
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			sbuffer->GetResource(),
			afterState,
			beforeState
		));
	}
};*/