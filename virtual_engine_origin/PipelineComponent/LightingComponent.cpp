#include "LightingComponent.h"
#include "PrepareComponent.h"
#include "CameraData/CameraTransformData.h"
#include "RenderPipeline.h"
#include "../Singleton/MathLib.h"
#include "../RenderComponent/ComputeShader.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../LogicComponent/DirectionalLight.h"
#include "../LogicComponent/Transform.h"
#include "../LogicComponent/DirectionalLight.h"
#include "../LogicComponent/World.h"
#include "CameraData/LightCBuffer.h"
#include "../Common/Camera.h"
using namespace Math;

#define XRES 32
#define YRES 16
#define ZRES 64
#define VOXELZ 64
#define MAXLIGHTPERCLUSTER 128
#define FROXELRATE 1.35
#define CLUSTERRATE 1.5

const ComputeShader* lightCullingShader;

namespace LightingComp_Namespace
{
	uint _LightIndexBuffer;
	uint _AllLight;
	uint LightCullCBufferID;

};
using namespace LightingComp_Namespace;


void LightingComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<PrepareComponent>();
	lights.reserve(50);
	prepareComp = RenderPipeline::GetComponent<PrepareComponent>();
	lightCullingShader = ShaderCompiler::GetComputeShader("LightCull");
	RenderTextureFormat xyPlaneFormat;
	xyPlaneFormat.colorFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	xyPlaneFormat.usage = RenderTextureUsage::ColorBuffer;
	xyPlaneTexture.New(
		device, XRES, YRES, xyPlaneFormat, TextureDimension::Tex3D, 4, 0, RenderTextureState::Unordered_Access);
	zPlaneTexture.New(device, ZRES, 2, xyPlaneFormat, TextureDimension::Tex2D, 1, 0, RenderTextureState::Unordered_Access
	);
	StructuredBufferElement ele;
	ele.elementCount = XRES * YRES * ZRES * (MAXLIGHTPERCLUSTER + 1);
	ele.stride = sizeof(uint);
	lightIndexBuffer.New(
		device, &ele, 1
	);
	cullingDescHeap.New(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true);
	xyPlaneTexture->BindUAVToHeap(cullingDescHeap, 0, device, 0);
	zPlaneTexture->BindUAVToHeap(cullingDescHeap, 1, device, 0);
	_LightIndexBuffer = ShaderID::PropertyToID("_LightIndexBuffer");
	_AllLight = ShaderID::PropertyToID("_AllLight");
	LightCullCBufferID = ShaderID::PropertyToID("LightCullCBuffer");


}
void LightingComponent::Dispose()
{
	xyPlaneTexture.Delete();
	zPlaneTexture.Delete();
	lightIndexBuffer.Delete();
	cullingDescHeap.Delete();
}
ArrayList<TemporalResourceCommand>& LightingComponent::SendRenderTextureRequire(const EventData& evt)
{
	return tempResources;
}

LightFrameData::LightFrameData(ID3D12Device* device) :
	lightsInFrustum(device, 50, false, sizeof(LightCommand))
{
}

LightCameraData::LightCameraData(ID3D12Device* device) :
	lightCBuffer(device, 3, true, sizeof(LightCullCBuffer))
{

}

struct LightingRunnable
{
	ThreadCommand* tcmd;
	ID3D12Device* device;
	Camera* cam;
	FrameResource* res;
	LightingComponent* selfPtr;
	PrepareComponent* prepareComp;
	float clusterLightFarPlane;
	uint frameIndex;
	void operator()()
	{
		ThreadCommandFollower follower(tcmd);
		TransitionBarrierBuffer* barrier = tcmd->GetBarrierBuffer();
		auto commandList = tcmd->GetCmdList();
		Vector4 vec[6];
		memcpy(vec, prepareComp->frustumPlanes, sizeof(XMVECTOR) * 6);
		Vector3 camForward = cam->GetLook();
		vec[0] = MathLib::GetPlane((camForward), (cam->GetPosition() + Min<double>(cam->GetFarZ(), clusterLightFarPlane) * camForward));
		Light::GetAllLightings(
			selfPtr->lights,
			selfPtr->lightPtrs,
			vec,
			(prepareComp->frustumMinPos),
			(prepareComp->frustumMaxPos));
		LightFrameData* lightData = (LightFrameData*)res->GetPerCameraResource(selfPtr, cam, [&]()->LightFrameData*
			{
				return new LightFrameData(device);
			});
		LightCameraData* camData = (LightCameraData*)cam->GetResource(selfPtr, [&]()->LightCameraData*
			{
				return new LightCameraData(device);
			});
		if (lightData->lightsInFrustum.GetElementCount() < selfPtr->lights.size())
		{
			uint maxSize = Max<size_t>(selfPtr->lights.size(), (uint)(lightData->lightsInFrustum.GetElementCount() * 1.5));
			lightData->lightsInFrustum.Create(device, maxSize, false, sizeof(LightCommand));
		}
		LightCullCBuffer cb;
		Vector3 farPos = cam->GetPosition() + clusterLightFarPlane * cam->GetLook();
		cb._CameraFarPos = farPos;
		cb._CameraFarPos.w = clusterLightFarPlane;
		Vector3 nearPos = cam->GetPosition() + cam->GetNearZ() * cam->GetLook();
		cb._CameraNearPos = nearPos;
		cb._CameraNearPos.w = cam->GetNearZ();
		cb._CameraForward = cam->GetLook();
		cb._LightCount = selfPtr->lights.size();
		//Test Sun Light
		DirectionalLight* dLight = DirectionalLight::GetInstance();
		if (dLight)
		{
			cb._SunColor = dLight->intensity * (Vector3)dLight->color;
			cb._SunEnabled = 1;
			cb._SunDir = -(Vector3)dLight->GetTransform()->GetForward();
			cb._SunShadowEnabled = 1;
		}
		else
		{
			cb._SunEnabled = 0;
			cb._SunColor = { 0,0,0 };
			cb._SunDir = { 0,0,0 };
			cb._SunShadowEnabled = 0;
		}
		void* mappedPtr = camData->lightCBuffer.GetMappedDataPtr(frameIndex);
		memcpy(mappedPtr, &cb, offsetof(LightCullCBuffer, _ShadowmapIndices));
		if (!selfPtr->lights.empty())
		{
			lightData->lightsInFrustum.CopyDatas(0, selfPtr->lights.size(), selfPtr->lights.data());
		}
		lightCullingShader->BindRootSignature(commandList, selfPtr->cullingDescHeap);
		lightCullingShader->SetResource<StructuredBuffer>(commandList, _LightIndexBuffer, selfPtr->lightIndexBuffer, 0);
		lightCullingShader->SetResource(commandList, _AllLight, &lightData->lightsInFrustum, 0);
		lightCullingShader->SetResource(commandList, LightCullCBufferID, &camData->lightCBuffer, frameIndex);
		lightCullingShader->SetResource<DescriptorHeap>(commandList, ShaderID::GetMainTex(), selfPtr->cullingDescHeap, 0);
		auto c = res->cameraCBs[cam->GetInstanceID()];
		lightCullingShader->SetResource(commandList, ShaderID::GetPerCameraBufferID(), c.buffer, c.element);
		lightCullingShader->Dispatch(commandList, 0, 1, 1, 1);//Set XY Plane
		lightCullingShader->Dispatch(commandList, 1, 1, 1, 1);//Set Z Plane

		barrier->UAVBarriers({
			selfPtr->xyPlaneTexture->GetResource(),
			selfPtr->zPlaneTexture->GetResource()
			});
		barrier->ExecuteCommand(commandList);
		lightCullingShader->Dispatch(commandList, 2, 1, 1, ZRES);
	}
};



void LightingComponent::RenderEvent(const EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<LightingRunnable>({
		commandList,
		data.device,
		data.camera,
		data.resource,
		this,
		prepareComp,
		128,
		data.ringFrameIndex
		});
}
#undef XRES
#undef YRES
#undef ZRES
#undef VOXELZ
#undef MAXLIGHTPERCLUSTER
#undef FROXELRATE
#undef CLUSTERRATE