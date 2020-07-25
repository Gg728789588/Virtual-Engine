#pragma once
#include "../RenderComponent/RenderTexture.h"
#include "../Common/d3dUtil.h"
#include "../RenderComponent/Shader.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../RenderComponent/UploadBuffer.h"
#include "../Singleton/ColorUtility.h"
#include "../RenderComponent/ComputeShader.h"
#include "../../CJsonObject/CJsonObject.hpp"
using namespace neb;
using namespace Math;
struct ColorGradingCBuffer
{
	float4 _Size; // x: lut_size, y: 1 / (lut_size - 1), zw: unused

	float4 _ColorBalance;
	float4 _ColorFilter;
	float4 _HueSatCon;

	float4 _ChannelMixerRed;
	float4 _ChannelMixerGreen;
	float4 _ChannelMixerBlue;

	float4 _Lift;
	float4 _InvGamma;
	float4 _Gain;
};
class ColorGrading
{
private:
	vengine::string jsonCache;
public:
	const int k_Lut2DSize = 32;
	const int k_Lut3DSize = 128;
	std::unique_ptr<RenderTexture> lut = nullptr;
	UploadBuffer cbuffer;
	CJsonObject* jsonData;
	DescriptorHeap heap;
	ColorGrading(ID3D12Device* device,
		CJsonObject* jsonData) :
		heap(
			device,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			1,
			true),
		cbuffer(
			device, 1,
			true, sizeof(ColorGradingCBuffer)),
		jsonData(jsonData)
	{
	}
	float temperature = 0.0f;
	float tint = 0.0f;
	
	float hueShift = 0;
	float saturation = 0;
	float contrast = 0;
	float mixerRedOutRedIn = 100;
	float mixerRedOutGreenIn = 0;
	float mixerRedOutBlueIn = 0;
	float mixerGreenOutRedIn = 0;
	float mixerGreenOutGreenIn = 100;
	float mixerGreenOutBlueIn = 0;
	float mixerBlueOutRedIn = 0;
	float mixerBlueOutGreenIn = 0;
	float mixerBlueOutBlueIn = 100;
	float postExposure = 1;
	Vector4 filterColor = { 1,1,1,1 };
	Vector4 lift = { 1.0f,1.0f,1.0f,0.0f };
	Vector4 gamma = { 1.0f,1.0f,1.0f,0.0f };
	Vector4 gain = { 1.0f,1.0f,1.0f,0.0f };

	bool operator()(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		TransitionBarrierBuffer* barrierBuffer)
	{
#define DEFINE_STR(name) static vengine::string name##_str = #name
#define KEYVALUE_PAIR(n, t) JsonKeyValuePair<n>(t##_str, t)
		DEFINE_STR(temperature);
		DEFINE_STR(tint);
		
		DEFINE_STR(hueShift);
		DEFINE_STR(saturation);
		DEFINE_STR(contrast);
		DEFINE_STR(mixerRedOutRedIn);
		DEFINE_STR(mixerRedOutGreenIn);
		DEFINE_STR(mixerRedOutBlueIn);
		DEFINE_STR(mixerGreenOutRedIn);
		DEFINE_STR(mixerGreenOutGreenIn);
		DEFINE_STR(mixerGreenOutBlueIn);
		DEFINE_STR(mixerBlueOutRedIn);
		DEFINE_STR(mixerBlueOutGreenIn);
		DEFINE_STR(mixerBlueOutBlueIn);
		DEFINE_STR(lift);
		DEFINE_STR(gamma);
		DEFINE_STR(gain);
		DEFINE_STR(enabled);
		DEFINE_STR(colorFilter);
		DEFINE_STR(postExposure);

		int enabled = 0;

		GetValuesFromJson(jsonData,
			KEYVALUE_PAIR(float, temperature),
			KEYVALUE_PAIR(float, tint),
			KEYVALUE_PAIR(float, hueShift),
			KEYVALUE_PAIR(float, saturation),
			KEYVALUE_PAIR(float, contrast),
			KEYVALUE_PAIR(float, mixerRedOutRedIn),
			KEYVALUE_PAIR(float, mixerRedOutGreenIn),
			KEYVALUE_PAIR(float, mixerRedOutBlueIn),
			KEYVALUE_PAIR(float, mixerGreenOutRedIn),
			KEYVALUE_PAIR(float, mixerGreenOutGreenIn),
			KEYVALUE_PAIR(float, mixerGreenOutBlueIn),
			KEYVALUE_PAIR(float, mixerBlueOutRedIn),
			KEYVALUE_PAIR(float, mixerBlueOutGreenIn),
			KEYVALUE_PAIR(float, mixerBlueOutBlueIn),
			KEYVALUE_PAIR(float, postExposure),
			KEYVALUE_PAIR(int, enabled)
		);
#undef DEFINE_STR
#undef KEYVALUE_PAIR
		if (!enabled) return false;
		jsonData->Get(colorFilter_str, jsonCache);
		ReadStringToVector<float4>(jsonCache.data(), jsonCache.size(), (float4*)&filterColor);
		jsonData->Get(lift_str, jsonCache);
		ReadStringToVector<float4>(jsonCache.data(), jsonCache.size(), (float4*)&lift);
		jsonData->Get(gamma_str, jsonCache);
		ReadStringToVector<float4>(jsonCache.data(), jsonCache.size(), (float4*)&gamma);
		jsonData->Get(gain_str, jsonCache);
		ReadStringToVector<float4>(jsonCache.data(), jsonCache.size(), (float4*)&gain);

		if (lut) return true;
		RenderTextureFormat format;
		format.usage = RenderTextureUsage::ColorBuffer;
		format.colorFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		lut = std::unique_ptr<RenderTexture>(new RenderTexture(
			device,
			128, 128,
			format,
			TextureDimension::Tex3D,
			128, 0, RenderTextureState::Unordered_Access
		));
		const ComputeShader* lutBakeShader = ShaderCompiler::GetComputeShader("Lut3DBaker");
		ColorGradingCBuffer& cb = *(ColorGradingCBuffer*)cbuffer.GetMappedDataPtr(0);
		cb._Size = { (float)k_Lut3DSize, 1.0f / (k_Lut3DSize - 1.0f), 0.0f, 0.0f };
		Vector3 colorBalance = ColorUtility::ComputeColorBalance(temperature, tint);
		cb._ColorBalance = colorBalance;
		cb._ColorFilter = (Vector4)filterColor;
		float hue = hueShift / 360.0f;         // Remap to [-0.5;0.5]
		float sat = saturation / 100.0f + 1.0f;  // Remap to [0;2]
		float con = contrast / 100.0f + 1.0f;    // Remap to [0;2]
		cb._HueSatCon = { hue, sat, con, 0.0f };
		Vector4 channelMixerR(mixerRedOutRedIn, mixerRedOutGreenIn, mixerRedOutBlueIn, 0.0f);
		Vector4 channelMixerG(mixerGreenOutRedIn, mixerGreenOutGreenIn, mixerGreenOutBlueIn, 0.0f);
		Vector4 channelMixerB(mixerBlueOutRedIn, mixerBlueOutGreenIn, mixerBlueOutBlueIn, 0.0f);
		cb._ChannelMixerRed = channelMixerR / 100.0f;
		cb._ChannelMixerGreen = channelMixerG / 100.0f;
		cb._ChannelMixerBlue = channelMixerB / 100.0f;
		Vector4 liftVec = ColorUtility::ColorToLift(lift * 0.2f);
		Vector4 gainVec = ColorUtility::ColorToGain(gain * 0.8f);
		Vector4 invgamma = ColorUtility::ColorToInverseGamma(gamma * 0.8f);
		cb._InvGamma = invgamma;
		cb._Lift = liftVec;
		cb._Gain = gainVec;
		uint groupSize = k_Lut3DSize / 4;
		lut->BindUAVToHeap(&heap, 0, device, 0);
		lutBakeShader->BindRootSignature(cmdList, &heap);
		lutBakeShader->SetResource(cmdList, ShaderID::PropertyToID("Params"), &cbuffer, 0);
		lutBakeShader->SetResource(cmdList, ShaderID::PropertyToID("_MainTex"), &heap, 0);
		lutBakeShader->Dispatch(cmdList, 0, groupSize, groupSize, groupSize);
		barrierBuffer->AddCommand(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ, lut->GetResource());
		return true;
	}
};