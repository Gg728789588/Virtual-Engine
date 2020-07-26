#include "VolumetricComponent.h"
#include "PrepareComponent.h"
#include "LightingComponent.h"
#include "../RenderComponent/Light.h"
#include "CameraData/CameraTransformData.h"
#include "RenderPipeline.h"
#include "../Singleton/MathLib.h"
#include "../RenderComponent/ComputeShader.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../LogicComponent/Transform.h"
#include "../LogicComponent/DirectionalLight.h"
#include "../LogicComponent/World.h"
#include "../Singleton/Graphics.h"
#include "CameraData/LightCBuffer.h"
#include "CSMComponent.h"
#include "LocalShadowComponent.h"
#include "../RenderComponent/RenderTexture.h"
#include "../Common/Camera.h"

namespace VolumetricGlobal
{
	LightingComponent* lightComp_Volume;
	PrepareComponent* prepareComp_VolumeComp;
	const uint3 FROXEL_RESOLUTION = { 160, 90, 128 };
	uint LightCullCBuffer_ID;
	uint FroxelParams;
	uint _GreyTex;
	uint _VolumeTex_ID_VolumeComponent;
	uint _LastVolume;
	uint _RWLastVolume;
	uint _GreyCubemap;
	const ComputeShader* froxelShader;

	float indirectIntensity = 0.16f;
	float darkerWeight = 0.75f;
	float brighterWeight = 0.85f;
	const uint marchStep = 64;
	float availableDistance = 32;
	float fogDensity = 0.0001;
}
#define _VolumeTex _VolumeTex_ID_VolumeComponent
std::unique_ptr<RenderTexture> VolumetricComponent::volumeRT;
using namespace VolumetricGlobal;
struct FroxelParamsStruct
{
	float4 _FroxelSize;
	float4 _VolumetricLightVar;
	float4 _TemporalWeight;
	float _LinearFogDensity;
};

VolumetricCameraData::VolumetricCameraData(ID3D12Device* device) :
	cbuffer(device, 3, true, sizeof(FroxelParamsStruct))
{
	RenderTextureFormat volumeFormat;
	volumeFormat.usage = RenderTextureUsage::ColorBuffer;
	volumeFormat.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	lastVolume = std::unique_ptr<RenderTexture>(new RenderTexture(
		device,
		FROXEL_RESOLUTION.x,
		FROXEL_RESOLUTION.y,
		volumeFormat,
		TextureDimension::Tex3D,
		FROXEL_RESOLUTION.z,
		0,
		RenderTextureState::Unordered_Access));
}
VolumetricCameraData::~VolumetricCameraData()
{

}
class VolumetricFrameData : public IPipelineResource
{
public:
	uint lastVolumeUAVGlobalIndex;
	uint volumeRTUAVGlobalIndex;
	VolumetricFrameData()
	{
		World* world = World::GetInstance();
		if (world)
		{
			lastVolumeUAVGlobalIndex = Graphics::GetDescHeapIndexFromPool();
			volumeRTUAVGlobalIndex = Graphics::GetDescHeapIndexFromPool();
		}
	}
	~VolumetricFrameData()
	{

		Graphics::ReturnDescHeapIndexToPool(lastVolumeUAVGlobalIndex);
		Graphics::ReturnDescHeapIndexToPool(volumeRTUAVGlobalIndex);
		
	}
};

struct VolumetricRunnable
{
	FrameResource* res;
	Camera* cam;
	ID3D12Device* device;
	ThreadCommand* tCmd;
	VolumetricComponent* selfPtr;
	uint frameIndex;
	static uint _AllLight;
	static uint _LightIndexBuffer;
	void operator()()
	{
		ThreadCommandFollower follower(tCmd);
		TransitionBarrierBuffer* barrier = tCmd->GetBarrierBuffer();
		auto commandList = tCmd->GetCmdList();
		VolumetricCameraData* camData = (VolumetricCameraData*)cam->GetResource(selfPtr, [&]()->VolumetricCameraData*
			{
				return new VolumetricCameraData(device);
			});
		VolumetricFrameData* frameData = (VolumetricFrameData*)res->GetPerCameraResource(selfPtr, cam, []()->VolumetricFrameData*
			{
				return new VolumetricFrameData;
			});
		LightFrameData* lightFrameData = (LightFrameData*)res->GetPerCameraResource(lightComp_Volume, cam, []()->LightFrameData*
			{
#ifndef NDEBUG
				throw "No Light Data Exception!";
#endif
				return nullptr;	//Get Error if there is no light coponent in pipeline
			});
		LightCameraData* lightData = (LightCameraData*)cam->GetResource(lightComp_Volume, [&]()->LightCameraData*
			{
				throw "Null!";
			});
		FroxelParamsStruct* params = (FroxelParamsStruct*)camData->cbuffer.GetMappedDataPtr(frameIndex);
		params->_VolumetricLightVar = { (float)cam->GetNearZ(), (float)(availableDistance - cam->GetNearZ()), availableDistance, indirectIntensity };
		params->_TemporalWeight = { darkerWeight, brighterWeight, 1, 1 };
		params->_FroxelSize = { (float)FROXEL_RESOLUTION.x, (float)FROXEL_RESOLUTION.y, (float)FROXEL_RESOLUTION.z , 1 };
		params->_LinearFogDensity = fogDensity;
		DescriptorHeap* heap = Graphics::GetGlobalDescHeap();
		froxelShader->BindRootSignature(commandList, heap);
		camData->lastVolume->BindUAVToHeap(heap, frameData->lastVolumeUAVGlobalIndex, device, 0);
		VolumetricComponent::volumeRT->BindUAVToHeap(heap, frameData->volumeRTUAVGlobalIndex, device, 0);
		froxelShader->SetResource(commandList, FroxelParams, &camData->cbuffer, frameIndex);
		froxelShader->SetResource(commandList, LightCullCBuffer_ID, &lightData->lightCBuffer, frameIndex);
		auto constBufferID = res->cameraCBs[cam->GetInstanceID()];
		froxelShader->SetResource(commandList, ShaderID::GetPerCameraBufferID(), constBufferID.buffer, constBufferID.element);
		froxelShader->SetResource(commandList, _GreyTex, heap, 0);
		froxelShader->SetResource(commandList, _GreyCubemap, heap, 0);
		froxelShader->SetResource(commandList, _VolumeTex, heap, VolumetricComponent::volumeRT->GetGlobalDescIndex());
		froxelShader->SetResource(commandList, _LastVolume, heap, camData->lastVolume->GetGlobalDescIndex());
		froxelShader->SetResource(commandList, _RWLastVolume, heap, frameData->lastVolumeUAVGlobalIndex);
		froxelShader->SetStructuredBufferByAddress(commandList, _AllLight, lightFrameData->lightsInFrustum.GetAddress(0));
		froxelShader->SetStructuredBufferByAddress(commandList, _LightIndexBuffer, lightComp_Volume->lightIndexBuffer->GetAddress(0, 0));

		uint3 dispatchCount = { FROXEL_RESOLUTION.x / 4, FROXEL_RESOLUTION.y / 2, FROXEL_RESOLUTION.z / marchStep };
		//Clear
		froxelShader->Dispatch(commandList, 2, dispatchCount.x, dispatchCount.y, dispatchCount.z);
		barrier->UAVBarrier( camData->lastVolume->GetResource());
		barrier->ExecuteCommand(commandList);
		//Draw Direct Light
		froxelShader->Dispatch(commandList, 0, dispatchCount.x, dispatchCount.y, dispatchCount.z);
		//Copy
		barrier->UAVBarrier(VolumetricComponent::volumeRT->GetResource());
		barrier->ExecuteCommand(commandList);
		froxelShader->Dispatch(commandList, 3, dispatchCount.x, dispatchCount.y, dispatchCount.z);
		barrier->UAVBarrier(VolumetricComponent::volumeRT->GetResource());
		barrier->ExecuteCommand(commandList);
		//Scatter
		froxelShader->Dispatch(commandList, 1, FROXEL_RESOLUTION.x / 160, FROXEL_RESOLUTION.y / 2, 1);
	}
};

uint VolumetricRunnable::_AllLight;
uint VolumetricRunnable::_LightIndexBuffer;

void VolumetricComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	VolumetricRunnable::_AllLight = ShaderID::PropertyToID("_AllLight");
	VolumetricRunnable::_LightIndexBuffer = ShaderID::PropertyToID("_LightIndexBuffer");
	lightComp_Volume = RenderPipeline::GetComponent<LightingComponent>();
	prepareComp_VolumeComp = RenderPipeline::GetComponent<PrepareComponent>();
	SetCPUDepending<LightingComponent>();
	SetGPUDepending<CSMComponent, LocalShadowComponent>();
	RenderTextureFormat volumeFormat;
	volumeFormat.usage = RenderTextureUsage::ColorBuffer;
	volumeFormat.colorFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	volumeRT = std::unique_ptr<RenderTexture>(new RenderTexture(
		device,
		FROXEL_RESOLUTION.x,
		FROXEL_RESOLUTION.y,
		volumeFormat,
		TextureDimension::Tex3D,
		FROXEL_RESOLUTION.z,
		0,
		RenderTextureState::Unordered_Access));
	LightCullCBuffer_ID = ShaderID::PropertyToID("LightCullCBuffer");
	FroxelParams = ShaderID::PropertyToID("FroxelParams");
	_GreyTex = ShaderID::PropertyToID("_GreyTex");
	_VolumeTex = ShaderID::PropertyToID("_VolumeTex");
	_LastVolume = ShaderID::PropertyToID("_LastVolume");
	_RWLastVolume = ShaderID::PropertyToID("_RWLastVolume");
	_GreyCubemap = ShaderID::PropertyToID("_GreyCubemap");
	froxelShader = ShaderCompiler::GetComputeShader("Froxel");
}

void VolumetricComponent::Dispose()
{
	volumeRT = nullptr;
}
ArrayList<TemporalResourceCommand>& VolumetricComponent::SendRenderTextureRequire(const EventData& evt)
{
	return tempResource;
}
void VolumetricComponent::RenderEvent(const EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<VolumetricRunnable>(
		{
			data.resource,
			data.camera,
			data.device,
			commandList,
			this,
			data.ringFrameIndex
		});
}
#undef _VolumeTex