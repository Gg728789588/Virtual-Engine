#include "LocalShadowComponent.h"
#include "LightingComponent.h"
#include "PrepareComponent.h"
#include "../RenderComponent/Light.h"
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
#include "../RenderComponent/Light.h"
#include "../Common/RenderPackage.h"
#include "../CubeRender/CubeDrawer.h"

using namespace Math;
ArrayList<TemporalResourceCommand>& LocalShadowComponent::SendRenderTextureRequire(const EventData& evt)
{
	return tempResources;
}

struct LocalLightFrameData : public IPipelineResource
{
	ArrayList<ConstBufferElement> tempEles;
	ArrayList<ConstBufferElement> cullEles;
	CBufferPool* cbPool;
	LocalLightFrameData(CBufferPool* cbPool) : cbPool(cbPool)
	{

	}
	ConstBufferElement GetCullDataBuffer(ID3D12Device* device)
	{
		auto ele = World::GetInstance()->GetCubeDrawer()->GetCullConstBuffer(device);
		cullEles.push_back(ele);
		return ele;
	}
	void Clear()
	{
		for (auto ite = tempEles.begin(); ite != tempEles.end(); ++ite)
		{
			cbPool->Return(*ite);
		}
		tempEles.clear();
		if (World::GetInstance() && !cullEles.empty())
		{
			World::GetInstance()->GetCubeDrawer()->ReturnCullConstBuffers(cullEles.data(), cullEles.size());
		}
		cullEles.clear();
	}
	~LocalLightFrameData()
	{
		Clear();
	}
};

struct LocalShadowRunnable
{
	ThreadCommand* tcmd;
	ID3D12Device* device;
	FrameResource* res;
	LightingComponent* lightCompPtr;
	LocalShadowComponent* selfPtr;
	Camera* cam;
	void operator()()
	{
		ThreadCommandFollower follower(tcmd);
		LocalLightFrameData* frameData = (LocalLightFrameData*)res->GetPerCameraResource(selfPtr, cam, [&]()->LocalLightFrameData* {
			return new LocalLightFrameData(selfPtr->localLightShadowmapData);
			});
		frameData->Clear();
		auto commandList = tcmd->GetCmdList();
		auto barrier = tcmd->GetBarrierBuffer();
		ArrayList<Light*>& value = lightCompPtr->lightPtrs;
		selfPtr->shadowCommands.clear();
		for (size_t i = 0; i < value.size(); ++i)
		{
			Light* ite = value[i];
			if (ite->ShadowEnabled())
			{
				if (!ite->updateEveryFrame)
				{
					if (ite->updateNextFrame)
					{
						ite->updateNextFrame = false;
					}
					else
					{
						continue;
					}
				}
				RenderTexture* rt = ite->GetShadowmap();

				Transform* tr = ite->GetTransform();
				LocalShadowComponent::ShadowCommand shadCommand;
				shadCommand.shadowmap = rt;
				shadCommand.lightIndex = i;
				switch (ite->GetLightType())
				{
				case Light::LightType_Spot:
				{
					shadCommand.type = LocalShadowComponent::ShadowType::Spot;
					float fov = ite->GetSpotAngle() * Deg2Rad;
					float farRange = ite->GetRange();
					float nearRange = ite->GetShadowNearPlane();
					//Matrix4 proj = XMMatrixPerspectiveFovLH(fov, 1, farRange, nearRange);
					Matrix4 view = GetInverseTransformMatrix(tr->GetRight(), tr->GetUp(), tr->GetForward(), tr->GetPosition());
					shadCommand.spotLight.view = view;
					shadCommand.spotLight.nearZ = nearRange;
					shadCommand.spotLight.farZ = farRange;
					shadCommand.spotLight.fov = fov;
					shadCommand.spotLight.right = tr->GetRight();
					shadCommand.spotLight.up = tr->GetUp();
					shadCommand.spotLight.forward = tr->GetForward();
					shadCommand.spotLight.pos = tr->GetPosition();
				}
				break;
				case Light::LightType_Point:
				{
					shadCommand.type = LocalShadowComponent::ShadowType::Cube;
					shadCommand.pointLight.nearClipPlane = ite->GetShadowNearPlane();
					shadCommand.pointLight.position = tr->GetPosition();
					shadCommand.pointLight.range = ite->GetRange();
				}
				break;
				}
				barrier->AddCommand(
					rt->GetReadState(),
					rt->GetWriteState(),
					rt->GetResource());
				selfPtr->shadowCommands.push_back(shadCommand);
			}
		}
		LightFrameData* lightData = (LightFrameData*)res->GetPerCameraResource(lightCompPtr, cam, [&]()->LightFrameData*
			{
				return nullptr;
			});

		RenderPackage pack(
			device,
			commandList,
			res,
			barrier,
			selfPtr->psoContainer);
		CubeDrawer* cubeDrawer = World::GetInstance()->GetCubeDrawer();
		Shader const* cubeShader = cubeDrawer->GetShader();
		for (auto ite = selfPtr->shadowCommands.begin(); ite != selfPtr->shadowCommands.end(); ++ite)
		{
			switch (ite->type)
			{
			case LocalShadowComponent::ShadowType::Spot:
			{

				Matrix4 proj = XMMatrixPerspectiveFovLH(ite->spotLight.fov, 1, ite->spotLight.farZ, ite->spotLight.nearZ);
				Matrix4 vp = mul(ite->spotLight.view, proj);
				Vector4 frustumPlanes[6];
				Vector3 frustumMinPos, frustumMaxPos;
				Vector3 right = ite->spotLight.right;
				Vector3 up = ite->spotLight.up;
				Vector3 forward = ite->spotLight.forward;
				Vector3 position = ite->spotLight.pos;
				MathLib::GetPerspFrustumPlanes(right, up, forward, position, ite->spotLight.fov, 1, ite->spotLight.nearZ, ite->spotLight.farZ, frustumPlanes);
				MathLib::GetFrustumBoundingBox(
					right, up, forward, position,
					2.0 * (double)ite->spotLight.nearZ * tan(0.5 * ite->spotLight.fov),
					2.0 * (double)ite->spotLight.farZ * tan(0.5 * ite->spotLight.fov),
					1,
					ite->spotLight.nearZ,
					ite->spotLight.farZ,
					&frustumMinPos,
					&frustumMaxPos
				);

				LightCommand* lightCmd = (LightCommand*)lightData->lightsInFrustum.GetMappedDataPtr(ite->lightIndex);
				lightCmd->spotVPMatrix = vp;
				ShadowmapDrawParam drawParams;
				drawParams._ShadowmapVP = vp;
				ConstBufferElement ele = frameData->cbPool->Get(device);
				frameData->tempEles.push_back(ele);
				memcpy(ele.buffer->GetMappedDataPtr(ele.element), &drawParams, sizeof(ShadowmapDrawParam));
				barrier->ExecuteCommand(commandList);
				ite->shadowmap->ClearRenderTarget(commandList, 0, 0);
				selfPtr->psoContainer->SetRenderTarget(commandList, { }, ite->shadowmap);
				cubeDrawer->ExecuteCull(pack, frameData->GetCullDataBuffer(device), cam, frustumPlanes, frustumMinPos, frustumMaxPos);
				cubeShader->BindRootSignature(commandList);
				cubeShader->SetResource(commandList, selfPtr->ProjectionShadowParams, ele.buffer, ele.element);
				cubeDrawer->DrawSpotLightShadow(pack);
			}
			break;
			case LocalShadowComponent::ShadowType::Cube:
			{
				Matrix4 viewProj[6];
				Vector4 frustumPlanesArray[36];
				Vector4* frustumPlanes[6] =
				{
					frustumPlanesArray,
					frustumPlanesArray + 6,
					frustumPlanesArray + 12,
					frustumPlanesArray + 18,
					frustumPlanesArray + 24,
					frustumPlanesArray + 30,
				};
				std::pair<Vector3, Vector3> minMaxPos[6];
				MathLib::GetCubemapRenderData(
					ite->pointLight.position,
					ite->pointLight.nearClipPlane,
					ite->pointLight.range,
					viewProj,
					(Vector4**)frustumPlanes,
					minMaxPos);
				for (uint i = 0; i < 6; ++i)
				{


					ShadowmapDrawParam drawParams;// = (ShadowmapDrawParam*)objectEle.buffer->GetMappedDataPtr(objectEle.element);
					drawParams._LightPos = (Vector3)ite->pointLight.position;
					drawParams._LightPos.w = ite->pointLight.range;
					drawParams._ShadowmapVP = viewProj[i];
					ConstBufferElement ele = frameData->cbPool->Get(device);
					frameData->tempEles.push_back(ele);
					memcpy(ele.buffer->GetMappedDataPtr(ele.element), &drawParams, sizeof(ShadowmapDrawParam));
					barrier->ExecuteCommand(commandList);

					selfPtr->psoContainer->SetRenderTarget(commandList, { RenderTarget(ite->shadowmap, i, 0) }, RenderTarget(selfPtr->cubemapDepth, 0, 0));
					ite->shadowmap->ClearRenderTarget(commandList, i, 0);
					selfPtr->cubemapDepth->ClearRenderTarget(commandList, 0, 0);
					cubeDrawer->ExecuteCull(pack, frameData->GetCullDataBuffer(device), cam, frustumPlanes[i], minMaxPos[i].first, minMaxPos[i].second);
					cubeShader->BindRootSignature(commandList);
					cubeShader->SetResource(commandList, selfPtr->ProjectionShadowParams, ele.buffer, ele.element);
					cubeDrawer->DrawPointLightShadow(pack);

				}
			}
			break;
			}
			barrier->AddCommand(
				ite->shadowmap->GetWriteState(),
				ite->shadowmap->GetReadState(),
				ite->shadowmap->GetResource());
		}
	}
};

void LocalShadowComponent::RenderEvent(const EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<LocalShadowRunnable>({
		commandList,
		data.device,
		data.resource,
		lightingComp,
		this,
		data.camera
		});
}
void LocalShadowComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	localLightShadowmapData.New(
		sizeof(ShadowmapDrawParam), 256, true
	);
	ProjectionShadowParams = ShaderID::PropertyToID("ProjectionShadowParams");
	SetCPUDepending<LightingComponent>();
	shadowCommands.reserve(50);
	lightingComp = RenderPipeline::GetComponent<LightingComponent>();
	psoContainer.New();

	cubemapDepth.New(device,
		1024, 1024,
		RenderTextureFormat::GetDepthFormat(RenderTextureDepthSettings_Depth16),
		TextureDimension::Tex2D, 1, 1);
	//cubemapDepth
}
void LocalShadowComponent::Dispose()
{
	localLightShadowmapData.Delete();
	psoContainer.Delete();
	cubemapDepth.Delete();
}