#include "PSOContainer.h"
#include "MeshLayout.h"
#include "Graphics.h"
#include "../RenderComponent/RenderTexture.h"
void PSOContainer::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	RenderTexture const* const* renderTargets,
	uint rtCount,
	RenderTexture const* depthTex)
{
	Graphics::SetRenderTarget(commandList, renderTargets, rtCount, depthTex);
	colorFormatCount = rtCount;
	for (uint i = 0; i < rtCount; ++i)
	{
		colorFormats[i] = renderTargets[i]->GetFormat();
	}
	if (depthTex)
		depthFormat = depthTex->GetFormat();
	else depthFormat = DXGI_FORMAT_UNKNOWN;
}
void PSOContainer::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	const std::initializer_list<RenderTexture const*>& renderTargets,
	RenderTexture const* depthTex)
{
	Graphics::SetRenderTarget(commandList, renderTargets, depthTex);
	colorFormatCount = renderTargets.size();
	for (uint i = 0; i < colorFormatCount; ++i)
	{
		colorFormats[i] = renderTargets.begin()[i]->GetFormat();
	}
	if (depthTex)
	depthFormat = depthTex->GetFormat();
	else depthFormat = DXGI_FORMAT_UNKNOWN;
}
void PSOContainer::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	const RenderTarget* renderTargets,
	uint rtCount,
	const RenderTarget& depth)
{
	Graphics::SetRenderTarget(commandList, renderTargets, rtCount, depth);
	colorFormatCount = rtCount;
	for (uint i = 0; i < rtCount; ++i)
	{
		colorFormats[i] = renderTargets[i].rt->GetFormat();
	}
	if (depth.rt)
	depthFormat = depth.rt->GetFormat();
	else depthFormat = DXGI_FORMAT_UNKNOWN;
}
void PSOContainer::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	const std::initializer_list<RenderTarget>& init,
	const RenderTarget& depth)
{
	Graphics::SetRenderTarget(commandList, init, depth);
	colorFormatCount = init.size();
	for (uint i = 0; i < colorFormatCount; ++i)
	{
		colorFormats[i] = init.begin()[i].rt->GetFormat();
	}
	if (depth.rt)
	depthFormat = depth.rt->GetFormat();
	else depthFormat = DXGI_FORMAT_UNKNOWN;
}
void PSOContainer::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	const RenderTarget* renderTargets,
	uint rtCount)
{
	Graphics::SetRenderTarget(commandList, renderTargets, rtCount);
	colorFormatCount = rtCount;
	for (uint i = 0; i < rtCount; ++i)
	{
		colorFormats[i] = renderTargets[i].rt->GetFormat();
	}
	depthFormat = DXGI_FORMAT_UNKNOWN;
}
void PSOContainer::SetRenderTarget(
	ID3D12GraphicsCommandList* commandList,
	const std::initializer_list<RenderTarget>& init)
{
	Graphics::SetRenderTarget(commandList, init);
	colorFormatCount = init.size();
	for (uint i = 0; i < colorFormatCount; ++i)
	{
		colorFormats[i] = init.begin()[i].rt->GetFormat();
	}
	depthFormat = DXGI_FORMAT_UNKNOWN;
}
PSOContainer::PSOContainer() : allPSOState(53), rtDict(11)
{
	settings.reserve(11);
}
uint PSOContainer::GetIndex()
{
	PSORTSetting rtSetting = PSORTSetting(colorFormats, colorFormatCount, depthFormat);
	auto ite = rtDict.Find(rtSetting);
	if (ite)
	{
		return ite.Value();
	}
	size_t count = settings.size();
	rtDict.Insert(rtSetting, count);
	settings.push_back(rtSetting);
	return count;
}
bool PSODescriptor::operator==(const PSODescriptor& other) const
{
	return other.shaderPtr == shaderPtr && other.shaderPass == shaderPass && other.meshLayoutIndex == meshLayoutIndex;
}

ID3D12PipelineState* PSOContainer::GetState(PSODescriptor& desc, ID3D12Device* device)
{
	auto index = GetIndex();
	desc.GenerateHash();
	PSORTSetting& set = settings[index];
	auto ite = allPSOState.Find(std::pair<uint, PSODescriptor>(index, desc));
	if (!ite)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
		ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		ArrayList<D3D12_INPUT_ELEMENT_DESC>* inputElement = MeshLayout::GetMeshLayoutValue(desc.meshLayoutIndex);
		opaquePsoDesc.InputLayout = { inputElement->data(), (UINT)inputElement->size() };
		desc.shaderPtr->GetPassPSODesc(desc.shaderPass, &opaquePsoDesc);
		opaquePsoDesc.SampleMask = UINT_MAX;
		opaquePsoDesc.PrimitiveTopologyType = desc.topology;
		opaquePsoDesc.NumRenderTargets = set.rtCount;
		memcpy(&opaquePsoDesc.RTVFormats, set.rtFormat, set.rtCount * sizeof(DXGI_FORMAT));
		opaquePsoDesc.SampleDesc.Count = 1;
		opaquePsoDesc.SampleDesc.Quality = 0;
		opaquePsoDesc.DSVFormat = set.depthFormat;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> result = nullptr;
		HRESULT testResult = device->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(result.GetAddressOf()));
		ThrowIfFailed(testResult);
		allPSOState.Insert(std::pair<uint, PSODescriptor>(index, desc), result);
		return result.Get();
	};
	return ite.Value().Get();

}

PSOContainer::PSORTSetting::PSORTSetting(DXGI_FORMAT* colorFormats, uint colorFormatCount, DXGI_FORMAT depthFormat) noexcept
{
	rtCount = Min<size_t>((size_t)8, colorFormatCount);
	memcpy(rtFormat, colorFormats, rtCount * sizeof(DXGI_FORMAT));
	this->depthFormat = depthFormat;
}
bool PSOContainer::PSORTSetting::operator==(const PSORTSetting& a) const noexcept
{
	if (depthFormat == a.depthFormat && rtCount == a.rtCount)
	{
		for (uint i = 0; i < rtCount; ++i)
		{
			if (rtFormat[i] != a.rtFormat[i]) return false;
		}
		return true;
	}
	return false;
}