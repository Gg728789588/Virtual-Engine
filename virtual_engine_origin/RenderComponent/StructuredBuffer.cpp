#include "StructuredBuffer.h"
#include "../Singleton/FrameResource.h"
#include "DescriptorHeap.h"
#include "UploadBuffer.h"
#include "../Singleton/Graphics.h"
StructuredBuffer::StructuredBuffer(
	ID3D12Device* device,
	StructuredBufferElement* elementsArray,
	UINT elementsCount,
	bool isIndirect,
	bool isReadable
) : elements(elementsCount), offsets(elementsCount)
{
	memcpy(elements.data(), elementsArray, sizeof(StructuredBufferElement) * elementsCount);
	uint64_t offst = 0;
	for (UINT i = 0; i < elementsCount; ++i)
	{
		offsets[i] = offst;
		auto& ele = elements[i];
		offst += ele.stride * ele.elementCount;
	}
	initState = isReadable ? D3D12_RESOURCE_STATE_GENERIC_READ : (isIndirect ? D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT : D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(offst, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		initState,
		nullptr,
		IID_PPV_ARGS(&Resource)
	));
	InitGlobalHeap(device);
}

void StructuredBuffer::BindSRVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.NumElements = elements[0].elementCount;
	srvDesc.Buffer.StructureByteStride = elements[0].stride;
	targetHeap->CreateSRV(device, this, &srvDesc, index);
}
void StructuredBuffer::BindUAVToHeap(DescriptorHeap* targetHeap, UINT index, ID3D12Device* device)const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	uavDesc.Buffer.NumElements = elements[0].elementCount;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.StructureByteStride = elements[0].stride;
	targetHeap->CreateUAV(device, this, &uavDesc, index);
}
void StructuredBuffer::InitGlobalHeap(ID3D12Device* device)
{
	srvDescIndex = Graphics::GetDescHeapIndexFromPool();
	uavDescIndex = Graphics::GetDescHeapIndexFromPool();
	BindSRVToHeap(Graphics::GetGlobalDescHeap(), srvDescIndex, device);
	BindUAVToHeap(Graphics::GetGlobalDescHeap(), uavDescIndex, device);
}
void StructuredBuffer::DisposeGlobalHeap() const
{
	Graphics::ReturnDescHeapIndexToPool(srvDescIndex);
	Graphics::ReturnDescHeapIndexToPool(uavDescIndex);
}

StructuredBuffer::StructuredBuffer(
	ID3D12Device* device,
	const std::initializer_list<StructuredBufferElement>& elementsArray,
	bool isIndirect,
	bool isReadable) :
	elements(elementsArray.size()), offsets(elementsArray.size())
{
	memcpy(elements.data(), elementsArray.begin(), sizeof(StructuredBufferElement) * elementsArray.size());
	uint64_t offst = 0;
	for (UINT i = 0; i < elementsArray.size(); ++i)
	{
		offsets[i] = offst;
		auto& ele = elements[i];
		offst += ele.stride * ele.elementCount;
	}
	initState = isReadable ? D3D12_RESOURCE_STATE_GENERIC_READ : (isIndirect ? D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT : D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(offst, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		initState,
		nullptr,
		IID_PPV_ARGS(&Resource)
	));
	InitGlobalHeap(device);
}

StructuredBuffer::~StructuredBuffer()
{
	DisposeGlobalHeap();
}

StructuredBuffer::StructuredBuffer(
	ID3D12Device* device,
	StructuredBufferElement* elementsArray,
	UINT elementsCount,
	D3D12_RESOURCE_STATES targetState
) : elements(elementsCount), offsets(elementsCount)
{
	memcpy(elements.data(), elementsArray, sizeof(StructuredBufferElement) * elementsCount);
	uint64_t offst = 0;
	for (UINT i = 0; i < elementsCount; ++i)
	{
		offsets[i] = offst;
		auto& ele = elements[i];
		offst += ele.stride * ele.elementCount;
	}
	initState = targetState;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(offst, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		initState,
		nullptr,
		IID_PPV_ARGS(&Resource)
	));
	InitGlobalHeap(device);
}
StructuredBuffer::StructuredBuffer(
	ID3D12Device* device,
	const std::initializer_list<StructuredBufferElement>& elementsArray,
	D3D12_RESOURCE_STATES targetState
) : elements(elementsArray.size()), offsets(elementsArray.size())
{
	memcpy(elements.data(), elementsArray.begin(), sizeof(StructuredBufferElement) * elementsArray.size());
	uint64_t offst = 0;
	for (UINT i = 0; i < elementsArray.size(); ++i)
	{
		offsets[i] = offst;
		auto& ele = elements[i];
		offst += ele.stride * ele.elementCount;
	}
	initState = targetState;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(offst, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		initState,
		nullptr,
		IID_PPV_ARGS(&Resource)
	));
	InitGlobalHeap(device);
}

void StructuredBuffer::TransformBufferState(ID3D12GraphicsCommandList* commandList, StructuredBufferState beforeState, StructuredBufferState afterState) const
{
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		Resource.Get(),
		(D3D12_RESOURCE_STATES)beforeState,
		(D3D12_RESOURCE_STATES)afterState
	));
}

void StructuredBuffer::UploadData(ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	FrameResource* res,
	uint element, uint startIndex,
	uint endIndex, void const* sourceData) const
{
#if defined(DEBUG) || defined(_DEBUG)
	if (endIndex > GetElementCount(element))
		throw "Out of Range!";
	if (startIndex >= endIndex)
		throw "Illegal Index Range!";
#endif
	UploadBuffer ubuffer(device, endIndex - startIndex, false, GetStride(element));
	if (res) ubuffer.ReleaseAfterFrame(res);
	else ubuffer.DelayReleaseAfterFrame();
	ubuffer.CopyDatas(0, ubuffer.GetElementCount(), sourceData);
	commandList->CopyBufferRegion(
		Resource.Get(), GetAddressOffset(element, startIndex),
		ubuffer.GetResource(), 0, ubuffer.GetElementCount() * ubuffer.GetStride()
	);
}
uint64_t StructuredBuffer::GetStride(UINT index) const
{
	return elements[index].stride;
}
uint64_t StructuredBuffer::GetElementCount(UINT index) const
{
	return elements[index].elementCount;
}
D3D12_GPU_VIRTUAL_ADDRESS StructuredBuffer::GetAddress(UINT element, UINT index) const
{

#ifdef NDEBUG
	auto& ele = elements[element];
	return Resource->GetGPUVirtualAddress() + offsets[element] + ele.stride * index;
#else
	if (element >= elements.size())
	{
		throw "Element Out of Range Exception";
	}
	auto& ele = elements[element];
	if (index >= ele.elementCount)
	{
		throw "Index Out of Range Exception";
	}

	return Resource->GetGPUVirtualAddress() + offsets[element] + ele.stride * index;
#endif
}

uint64_t StructuredBuffer::GetAddressOffset(UINT element, UINT index) const
{
#ifdef NDEBUG
	auto& ele = elements[element];
	return offsets[element] + ele.stride * index;
#else
	if (element >= elements.size())
	{
		throw "Element Out of Range Exception";
	}
	auto& ele = elements[element];
	if (index >= ele.elementCount)
	{
		throw "Index Out of Range Exception";
	}

	return offsets[element] + ele.stride * index;
#endif
}
