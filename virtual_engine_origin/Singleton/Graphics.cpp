#include "Graphics.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/Shader.h"
#include "MeshLayout.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../Singleton/PSOContainer.h"
#include "../RenderComponent/RenderTexture.h"
#include "../Common/Memory.h"
#define MAXIMUM_HEAP_COUNT 32768
StackObject<Mesh, true> Graphics::fullScreenMesh;
ObjectPtr<DescriptorHeap> Graphics::globalDescriptorHeap;
BitArray Graphics::usedDescs(MAXIMUM_HEAP_COUNT);
ArrayList<UINT> Graphics::unusedDescs(MAXIMUM_HEAP_COUNT);
std::mutex Graphics::mtx;
ObjectPtr<Mesh> Graphics::cubeMesh = nullptr;
ObjectPtr<Mesh> Graphics::sphereMesh = nullptr;
void Graphics::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	globalDescriptorHeap = ObjectPtr<DescriptorHeap>::NewObject(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MAXIMUM_HEAP_COUNT, true);
#pragma loop(hint_parallel(0))
	for (UINT i = 0; i < MAXIMUM_HEAP_COUNT; ++i)
	{
		unusedDescs[i] = i;
	}
	std::array<DirectX::XMFLOAT3, 3> vertex;
	std::array<DirectX::XMFLOAT2, 3> uv;
	vertex[0] = { -3, -1, 1 };
	vertex[1] = { 1, -1, 1 };
	vertex[2] = { 1, 3, 1 };
	uv[0] = { -1, 1 };
	uv[1] = { 1, 1 };
	uv[2] = { 1, -1 };
	std::array<INT16, 3> indices = { 0, 1, 2 };
	fullScreenMesh.New(
		3,
		vertex.data(),
		nullptr,
		nullptr,
		nullptr,
		uv.data(),
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		device,
		DXGI_FORMAT_R16_UINT,
		3,
		indices.data()
	);
	vengine::vector<ObjectPtr<Mesh>> resultMeshes;
	vengine::vector<vengine::string> meshPathes = { "Resource/Internal/Cube.vmesh", "Resource/Internal/Sphere.vmesh" };
	Mesh::LoadMeshFromFiles(meshPathes, device, true, true, false, true, true, false, false, false, resultMeshes);
	cubeMesh = resultMeshes[0];
	sphereMesh = resultMeshes[1];
}
UINT Graphics::GetDescHeapIndexFromPool()
{
	std::lock_guard lck(mtx);
	auto last = unusedDescs.end() - 1;
	UINT value = *last;
	unusedDescs.erase(last);
	usedDescs[value] = true;
	return value;
}

void Graphics::CopyTexture(
	ID3D12GraphicsCommandList* commandList,
	RenderTexture const* source, UINT sourceSlice, UINT sourceMipLevel,
	RenderTexture const* dest, UINT destSlice, UINT destMipLevel)
{
	if (source->GetDimension() == TextureDimension::Tex2D) sourceSlice = 0;
	if (dest->GetDimension() == TextureDimension::Tex2D) destSlice = 0;
	D3D12_TEXTURE_COPY_LOCATION sourceLocation;
	sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	sourceLocation.SubresourceIndex = sourceSlice * source->GetMipCount() + sourceMipLevel;
	D3D12_TEXTURE_COPY_LOCATION destLocation;
	destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destLocation.SubresourceIndex = destSlice * dest->GetMipCount() + destMipLevel;
	sourceLocation.pResource = source->GetResource();
	destLocation.pResource = dest->GetResource();
	commandList->CopyTextureRegion(
		&destLocation,
		0, 0, 0,
		&sourceLocation,
		nullptr
	);
}

void Graphics::CopyBufferToBCTexture(
	ID3D12GraphicsCommandList* commandList,
	UploadBuffer* sourceBuffer, size_t sourceBufferOffset,
	ID3D12Resource* textureResource, UINT targetMip,
	UINT width, UINT height, UINT depth, DXGI_FORMAT targetFormat, UINT pixelSize)
{
	D3D12_TEXTURE_COPY_LOCATION sourceLocation;
	sourceLocation.pResource = sourceBuffer->GetResource();
	sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	sourceLocation.PlacedFootprint.Offset = sourceBufferOffset;
	sourceLocation.PlacedFootprint.Footprint =
	{
		targetFormat, //DXGI_FORMAT Format;
		width, //UINT Width;
		height, //UINT Height;
		depth, //UINT Depth;
		d3dUtil::CalcConstantBufferByteSize(width * pixelSize * 4)//UINT RowPitch;
	};
	D3D12_TEXTURE_COPY_LOCATION destLocation;
	destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destLocation.SubresourceIndex = targetMip;
	destLocation.pResource = textureResource;
	commandList->CopyTextureRegion(
		&destLocation,
		0, 0, 0,
		&sourceLocation,
		nullptr
	);
}
void Graphics::CopyBufferToTexture(
	ID3D12GraphicsCommandList* commandList,
	UploadBuffer* sourceBuffer, size_t sourceBufferOffset,
	ID3D12Resource* textureResource, UINT targetMip,
	UINT width, UINT height, UINT depth, DXGI_FORMAT targetFormat, UINT pixelSize)
{
	D3D12_TEXTURE_COPY_LOCATION sourceLocation;
	sourceLocation.pResource = sourceBuffer->GetResource();
	sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	sourceLocation.PlacedFootprint.Offset = sourceBufferOffset;
	sourceLocation.PlacedFootprint.Footprint =
	{
		targetFormat, //DXGI_FORMAT Format;
		width, //UINT Width;
		height, //UINT Height;
		depth, //UINT Depth;
		d3dUtil::CalcConstantBufferByteSize(width * pixelSize)//UINT RowPitch;
	};
	D3D12_TEXTURE_COPY_LOCATION destLocation;
	destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destLocation.SubresourceIndex = targetMip;
	destLocation.pResource = textureResource;
	commandList->CopyTextureRegion(
		&destLocation,
		0, 0, 0,
		&sourceLocation,
		nullptr
	);
}

void Graphics::Blit(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	D3D12_CPU_DESCRIPTOR_HANDLE* renderTarget,
	DXGI_FORMAT* renderTargetFormats,
	UINT renderTargetCount,
	D3D12_CPU_DESCRIPTOR_HANDLE* depthTarget,
	DXGI_FORMAT depthFormat,
	PSOContainer* container,
	UINT width, UINT height,
	const Shader* shader, UINT pass)
{
	D3D12_VIEWPORT mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	D3D12_RECT mScissorRect = { 0, 0, (int)width, (int)height };
	PSODescriptor psoDesc;
	psoDesc.meshLayoutIndex = fullScreenMesh->GetLayoutIndex();
	psoDesc.shaderPass = pass;
	psoDesc.shaderPtr = shader;
	container->colorFormatCount = renderTargetCount;
	memcpy(container->colorFormats, renderTargetFormats, sizeof(DXGI_FORMAT) * renderTargetCount);
	container->depthFormat = depthFormat;
	commandList->OMSetRenderTargets(renderTargetCount, renderTarget, false, depthTarget);
	commandList->RSSetViewports(1, &mViewport);
	commandList->RSSetScissorRects(1, &mScissorRect);
	commandList->SetPipelineState(container->GetState(psoDesc, device));
	commandList->IASetVertexBuffers(0, 1, &fullScreenMesh->VertexBufferView());
	commandList->IASetIndexBuffer(&fullScreenMesh->IndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(fullScreenMesh->GetIndexCount(), 1, 0, 0, 0);
}


void Graphics::Blit(
	ID3D12GraphicsCommandList* commandList,
	ID3D12Device* device,
	const std::initializer_list<RenderTarget>& renderTarget,
	const RenderTarget& depthTarget,
	PSOContainer* container,
	const Shader* shader, UINT pass)
{
	uint width = 0;
	uint height = 0;
	const RenderTarget* rt = renderTarget.begin();
	uint renderTargetCount = renderTarget.size();

	if (renderTargetCount > 0)
	{

		width = rt->rt->GetWidth() >> rt->mipCount;
		height = rt->rt->GetHeight() >> rt->mipCount;
	}
	else if (depthTarget.rt)
	{
		width = depthTarget.rt->GetWidth() >> depthTarget.mipCount;
		height = depthTarget.rt->GetHeight() >> depthTarget.mipCount;
	}
	D3D12_VIEWPORT mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	D3D12_RECT mScissorRect = { 0, 0, (int)width, (int)height };
	PSODescriptor psoDesc;
	psoDesc.meshLayoutIndex = fullScreenMesh->GetLayoutIndex();
	psoDesc.shaderPass = pass;
	psoDesc.shaderPtr = shader;

	D3D12_CPU_DESCRIPTOR_HANDLE* depthHandlePtr = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE depthHandle;
	if (depthTarget.rt)
	{
		depthHandle = depthTarget.rt->GetColorDescriptor(depthTarget.depthSlice, depthTarget.mipCount);
		depthHandlePtr = &depthHandle;
	}
	container->SetRenderTarget(commandList, renderTarget, depthTarget);

	commandList->RSSetViewports(1, &mViewport);
	commandList->RSSetScissorRects(1, &mScissorRect);
	commandList->SetPipelineState(container->GetState(psoDesc, device));
	commandList->IASetVertexBuffers(0, 1, &fullScreenMesh->VertexBufferView());
	commandList->IASetIndexBuffer(&fullScreenMesh->IndexBufferView());
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(fullScreenMesh->GetIndexCount(), 1, 0, 0, 0);
}

void Graphics::ReturnDescHeapIndexToPool(UINT target)
{
	std::lock_guard lck(mtx);
	auto ite = usedDescs[target];
	if (ite)
	{
		unusedDescs.push_back(target);
		ite = false;
	}
}

void Graphics::ForceCollectAllHeapIndex()
{
	std::lock_guard lck(mtx);
	for (UINT i = 0; i < MAXIMUM_HEAP_COUNT; ++i)
	{
		unusedDescs[i] = i;
	}
	usedDescs.Reset(false);
}

void Graphics::DrawMesh(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	const Mesh* mesh,
	const Shader* shader, uint pass,
	PSOContainer* container, uint instanceCount)
{
	PSODescriptor desc;
	desc.meshLayoutIndex = mesh->GetLayoutIndex();
	desc.shaderPass = pass;
	desc.shaderPtr = shader;
	ID3D12PipelineState* pso = container->GetState(desc, device);
	commandList->SetPipelineState(pso);
	commandList->IASetVertexBuffers(0, 1, &mesh->VertexBufferView());
	commandList->IASetIndexBuffer(&mesh->IndexBufferView());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(mesh->GetIndexCount(), instanceCount, 0, 0, 0);
}

void Graphics::DrawMeshes(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	Mesh** mesh, uint meshCount,
	const Shader* shader, uint pass,
	PSOContainer* container, bool sort)
{
	if (meshCount == 0) return;
	if (sort)
	{
		auto compareFunc = [](Mesh* a, Mesh* b)->int32_t
		{
			return (int)a->GetLayoutIndex() - (int)b->GetLayoutIndex();
		};
		QuicksortStackCustomCompare<Mesh*, decltype(compareFunc)>(
			mesh, compareFunc, 0, meshCount - 1);
	}
	PSODescriptor desc;
	desc.meshLayoutIndex = mesh[0]->GetLayoutIndex();
	desc.shaderPass = pass;
	desc.shaderPtr = shader;
	ID3D12PipelineState* pso = container->GetState(desc, device);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetPipelineState(pso);
	for (uint i = 0; i < meshCount; ++i)
	{
		Mesh* m = mesh[i];
		if (m->GetLayoutIndex() != desc.meshLayoutIndex)
		{
			desc.meshLayoutIndex = m->GetLayoutIndex();
			pso = container->GetState(desc, device);
			commandList->SetPipelineState(pso);
		}
		commandList->IASetVertexBuffers(0, 1, &m->VertexBufferView());
		commandList->IASetIndexBuffer(&m->IndexBufferView());
		commandList->DrawIndexedInstanced(m->GetIndexCount(), 1, 0, 0, 0);
	}
}

void Graphics::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	RenderTexture const* const* renderTargets,
	uint rtCount,
	RenderTexture const* depthTex)
{
	D3D12_CPU_DESCRIPTOR_HANDLE* ptr = nullptr;
	if (rtCount > 0)
	{
		renderTargets[0]->SetViewport(commandList);
		ptr = (D3D12_CPU_DESCRIPTOR_HANDLE*)alloca(sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) * rtCount);
		for (uint i = 0; i < rtCount; ++i)
		{
			ptr[i] = renderTargets[i]->GetColorDescriptor(0, 0);
		}
	}
	else if (depthTex)
	{
		depthTex->SetViewport(commandList);
	}
	commandList->OMSetRenderTargets(rtCount, ptr, false, depthTex ? &depthTex->GetColorDescriptor(0, 0) : nullptr);
}
void Graphics::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	const std::initializer_list<RenderTexture const*>& rtLists,
	RenderTexture const* depthTex)
{
	RenderTexture const* const* renderTargets = rtLists.begin();
	size_t rtCount = rtLists.size();
	SetRenderTarget(
		commandList, renderTargets, rtCount,
		depthTex);
}

void Graphics::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	const RenderTarget* renderTargets,
	uint rtCount,
	const RenderTarget& depth)
{
	D3D12_CPU_DESCRIPTOR_HANDLE* ptr = nullptr;
	if (rtCount > 0)
	{
		renderTargets[0].rt->SetViewport(commandList, renderTargets[0].mipCount);
		ptr = (D3D12_CPU_DESCRIPTOR_HANDLE*)alloca(sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) * rtCount);
		for (uint i = 0; i < rtCount; ++i)
		{
			const RenderTarget& rt = renderTargets[i];
			ptr[i] = rt.rt->GetColorDescriptor(rt.depthSlice, rt.mipCount);
		}
	}
	else if (depth.rt)
	{
		depth.rt->SetViewport(commandList, depth.mipCount);
	}
	commandList->OMSetRenderTargets(rtCount, ptr, false, depth.rt ? &depth.rt->GetColorDescriptor(depth.depthSlice, depth.mipCount) : nullptr);
}
void Graphics::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	const std::initializer_list<RenderTarget>& init,
	const RenderTarget& depth)
{
	const RenderTarget* renderTargets = init.begin();
	const size_t rtCount = init.size();
	SetRenderTarget(commandList, renderTargets, rtCount, depth);
}
void Graphics::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	const RenderTarget* renderTargets,
	uint rtCount)
{
	if (rtCount > 0)
	{
		renderTargets[0].rt->SetViewport(commandList, renderTargets[0].mipCount);
		D3D12_CPU_DESCRIPTOR_HANDLE* ptr = (D3D12_CPU_DESCRIPTOR_HANDLE*)alloca(sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) * rtCount);
		for (uint i = 0; i < rtCount; ++i)
		{
			const RenderTarget& rt = renderTargets[i];
			ptr[i] = rt.rt->GetColorDescriptor(rt.depthSlice, rt.mipCount);
		}
		commandList->OMSetRenderTargets(rtCount, ptr, false, nullptr);
	}

}
void Graphics::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	const std::initializer_list<RenderTarget>& init)
{
	const RenderTarget* renderTargets = init.begin();
	const size_t rtCount = init.size();
	SetRenderTarget(commandList, renderTargets, rtCount);
}

#undef MAXIMUM_HEAP_COUNT