#pragma once
#include "../RenderComponent/Shader.h"
#include "../Struct/RenderTarget.h"
#include "../Common/HashMap.h"
#include <mutex>
class ThreadCommand;
class Graphics;
struct PSODescriptor
{
	size_t hash;
	const Shader* shaderPtr;
	UINT shaderPass;
	UINT meshLayoutIndex;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topology;
	bool operator==(const PSODescriptor& other)const;
	PSODescriptor() :
		topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
	{

	}
	void GenerateHash()
	{
		size_t value = shaderPass;
		value <<= 8;
		value &= meshLayoutIndex;
		value <<= 4;
		value &= topology;
		value <<= 4;
		value ^= reinterpret_cast<size_t>(shaderPtr);
		std::hash<size_t> h;
		hash = h(value);
	}
};

namespace std
{
	template <>
	struct hash<PSODescriptor>
	{
		size_t operator()(const PSODescriptor& key) const
		{
			return key.hash;
		}
	};
	template <>
	struct hash<std::pair<uint, PSODescriptor>>
	{
		size_t operator()(const std::pair<uint, PSODescriptor>& key) const
		{
			return key.first ^ key.second.hash;
		}
	};

}

class PSOContainer
{
	friend class Graphics;
private:
	struct PSORTSetting
	{
		DXGI_FORMAT depthFormat;
		UINT rtCount;
		DXGI_FORMAT rtFormat[8];
		PSORTSetting() {}
		PSORTSetting(DXGI_FORMAT* colorFormats, uint colorFormatCount, DXGI_FORMAT depthFormat) noexcept;
		bool operator==(const PSORTSetting& a) const noexcept;
		bool operator != (const PSORTSetting& a) const noexcept
		{
			return !operator==(a);
		}
	};

	struct hash_RTSetting
	{
		size_t operator()(const PSORTSetting& key) const
		{
			std::hash<DXGI_FORMAT> fmtHash;
			size_t h = fmtHash(key.depthFormat);
			for (uint i = 0; i < key.rtCount; ++i)
			{
				h >>= 4;
				h ^= fmtHash(key.rtFormat[i]);
			}
			return h;
		}
	};
	uint GetIndex();
	HashMap<std::pair<uint, PSODescriptor>, Microsoft::WRL::ComPtr<ID3D12PipelineState>> allPSOState;
	HashMap<PSORTSetting, uint, hash_RTSetting> rtDict;
	ArrayList<PSORTSetting> settings;
	DXGI_FORMAT colorFormats[8];
	uint colorFormatCount = 0;
	DXGI_FORMAT depthFormat = DXGI_FORMAT_UNKNOWN;
public:
	PSOContainer();
	void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		RenderTexture const* const* renderTargets,
		uint rtCount,
		RenderTexture const* depthTex = nullptr);
	void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		const std::initializer_list<RenderTexture const*>& renderTargets,
		RenderTexture const* depthTex = nullptr);
	void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		const RenderTarget* renderTargets,
		uint rtCount,
		const RenderTarget& depth);
	void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		const std::initializer_list<RenderTarget>& init,
		const RenderTarget& depth);
	void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		const RenderTarget* renderTargets,
		uint rtCount);
	void SetRenderTarget(
		ID3D12GraphicsCommandList* commandList,
		const std::initializer_list<RenderTarget>& init);
	
	ID3D12PipelineState* GetState(PSODescriptor& desc, ID3D12Device* device);
};