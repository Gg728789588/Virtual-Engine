#pragma once
#include "../Common/d3dUtil.h"
#include "../RenderComponent/ComputeShader.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../RenderComponent/UploadBuffer.h"
#include "../Common/MetaLib.h"
#include "../RenderComponent/RenderTexture.h"
#include "../Common/Camera.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../RenderComponent/StructuredBuffer.h"
#include "../Singleton/FrameResource.h"
#include "../Singleton/Graphics.h"
#include "../CJsonObject/CJsonObject.hpp"
using namespace neb;
class AutoExposure
{
	struct ParamsStruct
	{
		float4 _Params1; // x: lowPercent, y: highPercent, z: minBrightness, w: maxBrightness
		float4 _Params2; // x: speed down, y: speed up, z: exposure compensation, w: delta time
		float4 _ScaleOffsetRes; // x: scale, y: offset, w: histogram pass width, h: histogram pass height
		uint isFixed;
	};
	class AutoExposureCameraResource : public IPipelineResource
	{
	public:
		UploadBuffer paramBuffer;
		DescriptorHeap heap;
		StackObject<RenderTexture> texturePool[2];
		bool isReadable[2];
		inline static const uint PARAM_BUFFER_SIZE = 1;
		inline static const uint HEAP_SIZE = 4;
		
		AutoExposureCameraResource(ID3D12Device* device):
			paramBuffer(device, PARAM_BUFFER_SIZE * 3, true, sizeof(ParamsStruct)),
			heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, HEAP_SIZE * 3, true)
		{
			isReadable[0] = false;
			isReadable[1] = false;
			RenderTextureFormat format;
			format.usage = RenderTextureUsage::ColorBuffer;
			format.colorFormat = DXGI_FORMAT_R32_FLOAT;
			for (uint i = 0; i < _countof(texturePool); ++i)
			{
				texturePool[i].New(device,
					1, 1, format, TextureDimension::Tex2D, 1, 1, RenderTextureState::Unordered_Access);
			}
		}
		~AutoExposureCameraResource()
		{

			for (uint i = 0; i < _countof(texturePool); ++i)
			{
				texturePool[i].Delete();
			}
		}
	};
	const int rangeMin = -9; // ev
	const int rangeMax = 9; // ev
	const int k_Bins = 128;
	StackObject<StructuredBuffer> computeBuffer;
	uint pp = 0;
	uint _HistogramBuffer;
	uint _Source;
	uint _Destination;
	uint Params;
	float4 GetHistogramScaleOffsetRes(float width, float height)
	{
		float diff = rangeMax - rangeMin;
		float scale = 1.0f / diff;
		float offset = -rangeMin * scale;
		return float4(scale, offset, width, height);
	}
	// Don't forget to update 'ExposureHistogram.hlsl' if you change these values !

	int m_ThreadX;
	int m_ThreadY;
	bool firstFrame = true;
	const ComputeShader* histoShader;
	const 	ComputeShader* autoExpShader;
	CJsonObject* jsonData;
	vengine::string jsonValues;
public:
	AutoExposure(ID3D12Device* device, CJsonObject* jsonData) : 
		jsonData(jsonData)
	{
		jsonValues.reserve(10);
		Params = ShaderID::PropertyToID("Params");
		_Source = ShaderID::PropertyToID("_Source");
		_HistogramBuffer = ShaderID::PropertyToID("_HistogramBuffer");
		_Destination = ShaderID::PropertyToID("_Destination");
		m_ThreadX = 16;
		m_ThreadY = 16;

		histoShader = ShaderCompiler::GetComputeShader("ExposureHistogram");
		autoExpShader = ShaderCompiler::GetComputeShader("AutoExposure");


		StructuredBufferElement element[1] =
		{
			{sizeof(uint), (size_t)k_Bins}
		};
		computeBuffer.New(device, element, _countof(element));
	}
	~AutoExposure()
	{
		computeBuffer.Delete();
	}

	RenderTexture* Render(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		uint width, uint height,
		RenderTexture* source,
		Camera* cam,
		FrameResource* res,
		float deltaTime,
		TransitionBarrierBuffer* transitionBarrier,
		uint frameIndex)
	{
		float2 filtering = { 20, 95 };
		float minLuminance = -5;
		float maxLuminance = 5;
		float keyValue = 0.5;
		float speedUp = 6;
		float speedDown = 5;
		int enabled = 0;
		static vengine::string minLuminance_str = "minLuminance";
		static vengine::string filtering_str = "filtering";
		static vengine::string maxLuminance_str = "maxLuminance";
		static vengine::string speedUp_str = "speedUp";
		static vengine::string speedDown_str = "speedDown";
		static vengine::string keyValue_str = "keyValue";
		static vengine::string enabled_str = "enabled";
		GetValuesFromJson(
			jsonData,
			JsonKeyValuePair<float>(minLuminance_str, minLuminance),
			JsonKeyValuePair<float>(maxLuminance_str, maxLuminance),
			JsonKeyValuePair<float>(speedUp_str, speedUp),
			JsonKeyValuePair<float>(speedDown_str, speedDown),
			JsonKeyValuePair<float>(keyValue_str, keyValue),
			JsonKeyValuePair<int>(enabled_str, enabled),
			JsonKeyValuePair<vengine::string>(filtering_str, jsonValues)
		);
		if (!enabled) return nullptr;
		ReadStringToVector<float2>(jsonValues.data(), jsonValues.size(), &filtering);
		
		AutoExposureCameraResource* camRes = (AutoExposureCameraResource*)cam->GetResource(this, [&]()->AutoExposureCameraResource*
			{
				return new AutoExposureCameraResource(device);
			});
		//Generate Histogram
		histoShader->BindRootSignature(commandList, &camRes->heap);
		histoShader->SetStructuredBufferByAddress(commandList, _HistogramBuffer, computeBuffer->GetAddress(0, 0));
		ParamsStruct* ptr = (ParamsStruct*)camRes->paramBuffer.GetMappedDataPtr(AutoExposureCameraResource::PARAM_BUFFER_SIZE * frameIndex);
		ptr->isFixed = firstFrame;
		firstFrame = false;
		ptr->_ScaleOffsetRes = GetHistogramScaleOffsetRes(width, height);
		source->BindSRVToHeap(&camRes->heap, AutoExposureCameraResource::HEAP_SIZE * frameIndex, device);
		histoShader->SetResource(commandList, _Source, &camRes->heap, AutoExposureCameraResource::HEAP_SIZE * frameIndex);
		histoShader->SetResource(commandList, Params, &camRes->paramBuffer, AutoExposureCameraResource::PARAM_BUFFER_SIZE * frameIndex);
		histoShader->Dispatch(commandList, 1, (uint)ceil((k_Bins + 0.1f) / (float)m_ThreadX), 1, 1);
		transitionBarrier->UAVBarrier( computeBuffer->GetResource());
		uint2 dispatchCount = { (width + (2 * m_ThreadX - 1)) / (2 * m_ThreadX),
			(height + (2 * m_ThreadY - 1)) / (2 * m_ThreadY) };
		transitionBarrier->ExecuteCommand(commandList);
		histoShader->Dispatch(commandList, 0, dispatchCount.x, dispatchCount.y, 1);
		transitionBarrier->UAVBarrier(computeBuffer->GetResource());
		//Auto Exposure
		float lowPercent = filtering.x;
		float highPercent = filtering.y;
		const float kMinDelta = 1e-2f;
		highPercent = MathHelper::Clamp(highPercent, 1.0f + kMinDelta, 99.0f);
		lowPercent = MathHelper::Clamp(lowPercent, 1.0f, highPercent - kMinDelta);
		ptr->_Params1 = { lowPercent * 0.01f, highPercent * 0.01f, std::exp2(minLuminance), std::exp2(maxLuminance) };
		ptr->_Params2 = { speedDown, speedUp, keyValue, deltaTime };
		autoExpShader->BindRootSignature(commandList, &camRes->heap);
		autoExpShader->SetStructuredBufferByAddress(commandList, _HistogramBuffer, computeBuffer->GetAddress(0, 0));

		autoExpShader->SetResource(commandList, Params, &camRes->paramBuffer, AutoExposureCameraResource::PARAM_BUFFER_SIZE * frameIndex);
		autoExpShader->SetResource(commandList, _Source, &camRes->heap, 1 + AutoExposureCameraResource::HEAP_SIZE * frameIndex);
		autoExpShader->SetResource(commandList, _Destination, &camRes->heap, 2 + AutoExposureCameraResource::HEAP_SIZE * frameIndex);
		transitionBarrier->AddCommand(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ, computeBuffer->GetResource());
		RenderTexture* result = nullptr;
		if (ptr->isFixed)
		{
			camRes->texturePool[0]->BindUAVToHeap(&camRes->heap, 2 + AutoExposureCameraResource::HEAP_SIZE * frameIndex, device, 0);
			camRes->texturePool[1]->BindUAVToHeap(&camRes->heap, 3 + AutoExposureCameraResource::HEAP_SIZE * frameIndex, device, 0);
			transitionBarrier->ExecuteCommand(commandList);
			autoExpShader->Dispatch(commandList, 0, 1, 1, 1);
			if (!camRes->isReadable[0])
			{
				camRes->isReadable[0] = true;
				transitionBarrier->AddCommand(
					camRes->texturePool[0]->GetInitState(),
					camRes->texturePool[0]->GetReadState(),
					camRes->texturePool[0]->GetResource()
				);
			}
			result = camRes->texturePool[0];
		}
		else
		{
			RenderTexture* source = camRes->texturePool[++pp % 2];
			source->BindSRVToHeap(&camRes->heap, 1 + AutoExposureCameraResource::HEAP_SIZE * frameIndex, device);//source
			uint destIndex = ++pp % 2;
			RenderTexture* dest = camRes->texturePool[destIndex];
			dest->BindUAVToHeap(&camRes->heap, 2 + AutoExposureCameraResource::HEAP_SIZE * frameIndex, device, 0);//dest
			if (camRes->isReadable[destIndex])
			{
				camRes->isReadable[destIndex] = false;
				transitionBarrier->AddCommand(
					dest->GetReadState(),
					dest->GetInitState(),
					dest->GetResource());
			}
			transitionBarrier->ExecuteCommand(commandList);
			autoExpShader->Dispatch(commandList, 0, 1, 1, 1);
			++pp;
			pp %= 2;
			if (!camRes->isReadable[destIndex])
			{
				camRes->isReadable[destIndex] = true;
				transitionBarrier->AddCommand(
					dest->GetInitState(),
					dest->GetReadState(),
					dest->GetResource());
			}
			result = dest;
		}
		transitionBarrier->AddCommand(D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, computeBuffer->GetResource());
		
		return result;
	}

};