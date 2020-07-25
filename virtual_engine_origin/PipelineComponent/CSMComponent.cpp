#include "CSMComponent.h"
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
#include "../Common/RenderPackage.h"
#include "../LogicComponent/World.h"
#include "../Singleton/Graphics.h"
#include "../Singleton/PSOContainer.h"
#include "CameraData/LightCBuffer.h"
#include "../Common/Camera.h"

namespace CSM
{
	LightingComponent* lightComp;
	const float zDepth = 500;
	uint ProjectionShadowParams;
	StackObject<PSOContainer> container;
}
using namespace Math;
struct ShadowmapData
{
	Matrix4 vpMatrix;
	Vector4 position;
	float size;
};
Math::Matrix4 GetCSMTransformMat(const Math::Vector3& right, const Math::Vector3& up, const Math::Vector3& forward, const Math::Vector3& position)
{
	Math::Matrix4 target;
	target[0] = right;
	target[1] = up;
	target[2] = forward;
	target[3] = { position.GetX(),position.GetY(),position.GetZ(),1 };
	target[0].m128_f32[3] = 0;
	target[1].m128_f32[3] = 0;
	target[2].m128_f32[3] = 0;
	return target;
}
void GetCascadeShadowmapMatrices(
	const Vector3& sunRight,
	const Vector3& sunUp,
	const Vector3& sunForward,
	const Vector3& cameraRight,
	const Vector3& cameraUp,
	const Vector3& cameraForward,
	const Vector3& cameraPosition,
	float fov,
	float aspect,
	float* distances,
	uint distanceCount,
	ShadowmapData* results,
	Vector4* lastPositions,
	uint* shadowResolutions)
{

	Matrix4 sunLocalToWorld = transpose(GetCSMTransformMat(
		sunRight,
		sunUp,
		sunForward,
		Vector3(0, 0, 0)
	));

	Vector3 corners[8];
	Vector3* lastCorners = corners;
	Vector3* nextCorners = corners + 4;
	MathLib::GetCameraNearPlanePoints(
		cameraRight,
		cameraUp,
		cameraForward,
		cameraPosition,
		fov,
		aspect, distances[0], nextCorners);
	for (uint i = 0; i < distanceCount; ++i)
	{
		{
			Vector3* swaper = lastCorners;
			lastCorners = nextCorners;
			nextCorners = swaper;
		}
		MathLib::GetCameraNearPlanePoints(
			cameraRight,
			cameraUp,
			cameraForward,
			cameraPosition,
			fov,
			aspect, distances[i + 1], nextCorners);
		float farDist = distance(nextCorners[0], nextCorners[3]);
		float crossDist = distance(lastCorners[0], nextCorners[3]);
		float maxDist = Max(farDist, crossDist);
		Matrix4 projMatrix = XMMatrixOrthographicLH(maxDist, maxDist, 0,-CSM::zDepth);
		Matrix4 sunWorldToLocal = inverse(sunLocalToWorld);
		Vector4 minBoundingPoint, maxBoundingPoint;
		{
			minBoundingPoint = mul(sunWorldToLocal, Vector4(corners[0], 1));
			maxBoundingPoint = minBoundingPoint;
		}
		for (uint j = 1; j < 8; ++j)
		{
			Vector4 pointLocalPos = mul(sunWorldToLocal, Vector4(corners[j], 1));
			minBoundingPoint = Min(pointLocalPos, minBoundingPoint);
			maxBoundingPoint = Max(pointLocalPos, maxBoundingPoint);
		}
		Vector4 localPosition = (minBoundingPoint + maxBoundingPoint) * 0.5f;
		localPosition.SetZ(maxBoundingPoint.GetZ());
		Vector4 positionMovement = localPosition - lastPositions[i];
		float pixelLength = (maxDist) / shadowResolutions[i];
		positionMovement = floor(positionMovement / pixelLength) * pixelLength;
		localPosition = lastPositions[i] + positionMovement;
		localPosition.SetW(1);
		Vector3 position = (Vector3)mul(sunLocalToWorld, localPosition);
		lastPositions[i] = localPosition;
		Matrix4 viewMat = inverse(GetCSMTransformMat(
			sunRight,
			sunUp,
			sunForward,
			position));
		results[i] =
		{
			mul(viewMat, projMatrix),
			position,
			maxDist * 0.5f
		};
	}
}
class CSMLastDirectLightPosition : public IPipelineResource
{
public:
	UploadBuffer csmParamBuffer;
	Vector4 sunPositions[DirectionalLight::CascadeLevel];
	CSMLastDirectLightPosition(ID3D12Device* device) :
		csmParamBuffer(device, DirectionalLight::CascadeLevel * 3, true, sizeof(ShadowmapDrawParam))
	{
		memset(sunPositions, 0, sizeof(Vector3) * DirectionalLight::CascadeLevel);
	}
};
class CSMRunnable
{
public:

	Camera* cam;
	FrameResource* res;
	ThreadCommand* tCmd;
	CSMComponent* selfPtr;
	ID3D12Device* device;
	World* world;
	uint frameIndex;
	void operator()()
	{
		ThreadCommandFollower follower(tCmd);
		auto commandList = tCmd->GetCmdList();
		RenderPackage package(device, commandList, res, tCmd->GetBarrierBuffer(), CSM::container);
		DirectionalLight* dLight = DirectionalLight::GetInstance();
		if (dLight)
		{
			float dist[DirectionalLight::CascadeLevel + 1];
			ShadowmapData vpMatrices[DirectionalLight::CascadeLevel];
			dist[0] = cam->GetNearZ();
			memcpy(dist + 1, &dLight->shadowDistance, sizeof(float) * DirectionalLight::CascadeLevel);
			Vector3 sunRight = dLight->GetTransform()->GetRight();
			Vector3 sunUp = dLight->GetTransform()->GetUp();
			Vector3 sunForward = dLight->GetTransform()->GetForward();
			sunForward = -sunForward;
			CSMLastDirectLightPosition* lastLightData = (CSMLastDirectLightPosition*)cam->GetResource(selfPtr, [&]()->CSMLastDirectLightPosition*
				{
					return new CSMLastDirectLightPosition(device);
				});
			uint shadowReses[DirectionalLight::CascadeLevel] =
			{
				dLight->GetShadowmapResolution(0),
				dLight->GetShadowmapResolution(1),
				dLight->GetShadowmapResolution(2),
				dLight->GetShadowmapResolution(3)
			};
			GetCascadeShadowmapMatrices(
				sunRight,
				sunUp,
				sunForward,
				cam->GetRight(),
				cam->GetUp(),
				cam->GetLook(),
				cam->GetPosition(),
				cam->GetFovY(),
				cam->GetAspect(),
				dist,
				DirectionalLight::CascadeLevel,
				vpMatrices,
				lastLightData->sunPositions,
				shadowReses);
			LightCameraData* camData = (LightCameraData*)cam->GetResource(CSM::lightComp, [&]()->LightCameraData*
				{
					return new LightCameraData(device);
				});

			LightCullCBuffer cb;
			cb._CascadeDistance = {
				dLight->shadowDistance[0],
				dLight->shadowDistance[1],
				dLight->shadowDistance[2],
				dLight->shadowDistance[3]
			};
			cb._ShadowmapIndices = {
				dLight->GetShadowmap(0)->GetGlobalDescIndex(),
				dLight->GetShadowmap(1)->GetGlobalDescIndex(),
				dLight->GetShadowmap(2)->GetGlobalDescIndex(),
				dLight->GetShadowmap(3)->GetGlobalDescIndex()
			};
			cb._ShadowSoftValue =
			{
				dLight->shadowSoftValue[0] / dLight->GetShadowmapResolution(0),
				dLight->shadowSoftValue[1] / dLight->GetShadowmapResolution(1),
				dLight->shadowSoftValue[2] / dLight->GetShadowmapResolution(2),
				dLight->shadowSoftValue[3] / dLight->GetShadowmapResolution(3)
			};
			cb._ShadowOffset =
			{
				dLight->shadowBias[0] / CSM::zDepth,
				dLight->shadowBias[1] / CSM::zDepth,
				dLight->shadowBias[2] / CSM::zDepth,
				dLight->shadowBias[3] / CSM::zDepth
			};
			Vector4 frustumPlanes[6];
			Vector3 frustumPoints[8];
			//	float4 frustumPInFloat[6];
			Vector3 frustumMinPoint, frustumMaxPoint;
			for (uint i = 0; i < DirectionalLight::CascadeLevel; ++i)
			{
				auto shadowmap = dLight->GetShadowmap(i);
				tCmd->GetBarrierBuffer()->AddCommand(shadowmap->GetReadState(), shadowmap->GetWriteState(), shadowmap->GetResource());
			}
			tCmd->GetBarrierBuffer()->ExecuteCommand(commandList);
			for (uint i = 0; i < DirectionalLight::CascadeLevel; ++i)
			{
				auto& m = vpMatrices[i];
				MathLib::GetOrthoCamFrustumPlanes(sunRight, sunUp, sunForward, (Vector3&)m.position, m.size, m.size, -CSM::zDepth, 0, frustumPlanes);
				MathLib::GetOrthoCamFrustumPoints(sunRight, sunUp, sunForward, (Vector3&)m.position, m.size, m.size, -CSM::zDepth, 0, frustumPoints);

				auto shadowmap = dLight->GetShadowmap(i);

				frustumMinPoint = frustumPoints[0];
				frustumMaxPoint = frustumMinPoint;
				for (uint j = 1; j < 8; ++j)
				{
					frustumMinPoint = Min(frustumMinPoint, frustumPoints[j]);
					frustumMaxPoint = Max(frustumMaxPoint, frustumPoints[j]);
				}


				ShadowmapDrawParam* drawParams = (ShadowmapDrawParam*)lastLightData->csmParamBuffer.GetMappedDataPtr(i + DirectionalLight::CascadeLevel * frameIndex);
				drawParams->_ShadowmapVP = m.vpMatrix;
				cb._ShadowMatrix[i] = drawParams->_ShadowmapVP;

				tCmd->GetBarrierBuffer()->ExecuteCommand(commandList);
				CSM::container->SetRenderTarget(commandList, {}, shadowmap);
				shadowmap->ClearRenderTarget(commandList, 0, 0);

				auto v = i + DirectionalLight::CascadeLevel * frameIndex;
				//TODO
				//Draw Heres
			}
			for (uint i = 0; i < DirectionalLight::CascadeLevel; ++i)
			{
				auto shadowmap = dLight->GetShadowmap(i);
				tCmd->GetBarrierBuffer()->AddCommand(shadowmap->GetWriteState(), shadowmap->GetReadState(), shadowmap->GetResource());
			}
			LightCullCBuffer* mappedPtr = (LightCullCBuffer*)camData->lightCBuffer.GetMappedDataPtr(frameIndex);
			memcpy(&mappedPtr->_ShadowmapIndices, &cb._ShadowmapIndices, sizeof(LightCullCBuffer) - offsetof(LightCullCBuffer, _ShadowmapIndices));
		}
	}
};

void CSMComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	CSM::lightComp = RenderPipeline::GetComponent<LightingComponent>();
	SetCPUDepending<PrepareComponent>();
	CSM::ProjectionShadowParams = ShaderID::PropertyToID("ProjectionShadowParams");
	CSM::container.New();
}
void CSMComponent::Dispose()
{
	CSM::container.Delete();
}
ArrayList<TemporalResourceCommand>& CSMComponent::SendRenderTextureRequire(const EventData& evt)
{
	return tempResources;
}
void CSMComponent::RenderEvent(const EventData& data, ThreadCommand* commandList)
{
	ScheduleJob< CSMRunnable>(
		{
			data.camera,
			data.resource,
			commandList,
			this,
			data.device,
			data.world,
			data.ringFrameIndex
		});
}