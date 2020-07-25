#include "PrepareComponent.h"
#include "../Common/Camera.h"
#include "../RenderComponent/UploadBuffer.h"
#include "CameraData/CameraTransformData.h"
#include "../Singleton/MathLib.h"
#include "../Utility/Random.h"
#include "../CubeRender/CubeDrawer.h"
#include "../LogicComponent/World.h"
using namespace Math;
namespace PrepareComp
{
	UINT sampleIndex = 0;
	const UINT k_SampleCount = 8;
	Random randomComp;
}
using namespace PrepareComp;
double GetHalton(int index, int radix)
{
	double result = 0;
	double fraction = 1.0 / (double)radix;

	while (index > 0)
	{
		result += (double)(index % radix) * fraction;

		index /= radix;
		fraction /= (double)radix;
	}

	return result;
}
XMVECTOR GenerateRandomOffset()
{
	XMVECTOR offset = {
		GetHalton((sampleIndex & 1023) + 1, 2) - 0.5,
		GetHalton((sampleIndex & 1023) + 1, 3) - 0.5,
		0, 0
	};

	if (++sampleIndex >= k_SampleCount)
		sampleIndex = 0;

	return offset;
}

void GetJitteredPerspectiveProjectionMatrix(
	double nearClipPlane,
	double farClipPlane,
	double fieldOfView,
	double aspect,
	UINT pixelWidth, UINT pixelHeight,
	XMFLOAT2 offset, XMMATRIX* projectionMatrix)
{
	double nearPlane = nearClipPlane;
	double vertical = tan(0.5 * Deg2Rad * fieldOfView) * nearPlane;
	double horizontal = vertical * aspect;

	offset.x *= horizontal / (0.5f * pixelWidth);
	offset.y *= vertical / (0.5f * pixelHeight);

	XMFLOAT4X4* projPt = (XMFLOAT4X4*)projectionMatrix;
	projPt->m[2][0] -= offset.x / horizontal;
	projPt->m[2][1] += offset.y / vertical;		//TODD: dont know difference between ogl & dx
}
void GetJitteredProjectionMatrix(
	double nearClipPlane,
	double farClipPlane,
	double fieldOfView,
	double aspect,
	double jitterOffset,
	UINT pixelWidth, UINT pixelHeight,
	XMMATRIX& projectionMatrix,
	XMVECTOR& jitter)
{
	jitter = GenerateRandomOffset();
	jitter *= jitterOffset;
	XMFLOAT2* jitterPtr = (XMFLOAT2*)&jitter;
	GetJitteredPerspectiveProjectionMatrix(
		nearClipPlane, farClipPlane,
		fieldOfView,
		aspect,
		pixelWidth, pixelHeight,
		*jitterPtr,
		&projectionMatrix
	);
	*jitterPtr = { jitterPtr->x / pixelWidth, jitterPtr->y / pixelHeight };
}
void ConfigureJitteredProjectionMatrix(Camera* camera, UINT height, UINT width, double jitterOffset, CameraTransformData* data, Vector3 worldMoving, bool isWorldMoving)
{
	Matrix4 p = camera->GetProj();
	Matrix4 flipP = p;
	flipP[1].m128_f32[1] *= -1;
	data->nonJitteredProjMatrix = p;
	data->nonJitteredFlipProj = flipP;

	data->lastFrameJitter = data->jitter;
	XMVECTOR jitterVec = XMLoadFloat2(&data->jitter);
	GetJitteredProjectionMatrix(
		camera->GetNearZ(),
		camera->GetFarZ(),
		camera->GetFovY(),
		camera->GetAspect(),
		jitterOffset,
		width, height,
		(XMMATRIX&)p,
		jitterVec
	);
	memcpy(&data->jitter, &jitterVec, sizeof(XMFLOAT2));
	camera->SetProj(p);
	Matrix4 view = camera->GetView();
	if (isWorldMoving)
	{
		Matrix4 newLastView = GetInverseTransformMatrix(data->lastCameraRight, data->lastCameraUp, data->lastCameraForward, data->lastCameraPosition + worldMoving);
		data->lastNonJitterVP = mul(newLastView, data->lastProj);
		data->lastFlipNonJitterVP = mul(newLastView, data->lastFlipProj);
	}
	else
	{
		data->lastNonJitterVP = data->nonJitteredVPMatrix;
		data->lastFlipNonJitterVP = data->nonJitteredFlipVP;
	}
	data->lastCameraForward = camera->GetLook();
	data->lastCameraRight = camera->GetRight();
	data->lastCameraUp = camera->GetUp();
	data->lastCameraPosition = camera->GetPosition();
	data->lastProj = data->nonJitteredProjMatrix;
	data->lastFlipProj = data->nonJitteredFlipProj;
	data->nonJitteredVPMatrix = mul(view, data->nonJitteredProjMatrix);
	data->nonJitteredFlipVP = mul(view, data->nonJitteredFlipProj);
}
struct PrepareRunnable
{
	ID3D12Device* device;
	ThreadCommand* tcmd;
	PrepareComponent* ths;
	FrameResource* resource;
	Camera* camera;
	UINT width, height;
	float3 worldMovingDir;
	bool isMovingTheWorld;
	void operator()()
	{
		ThreadCommandFollower follower(tcmd);
		RenderPackage package(
			device,
			tcmd->GetCmdList(),
			resource,
			tcmd->GetBarrierBuffer(),
			nullptr);
		World::GetInstance()->GetCubeDrawer()->ExecuteCopyCommand(
			package, camera
		);
		CameraTransformData* transData = (CameraTransformData*)camera->GetResource(ths, []()->CameraTransformData*
			{
				return new CameraTransformData;
			});

		ConfigureJitteredProjectionMatrix(
			camera,
			width, height,
			1, transData,
			worldMovingDir, isMovingTheWorld
		);
		ths->_ZBufferParams.x = (float)(1.0 - (camera->GetFarZ() / camera->GetNearZ()));
		ths->_ZBufferParams.y = (float)(camera->GetFarZ() / camera->GetNearZ());
		ths->_ZBufferParams.y += ths->_ZBufferParams.x;
		ths->_ZBufferParams.x *= -1;
		ths->_ZBufferParams.z = ths->_ZBufferParams.x / camera->GetFarZ();
		ths->_ZBufferParams.w = ths->_ZBufferParams.y / camera->GetFarZ();

		//ths->_ZBufferParams.w += ths->_ZBufferParams.z;
		//ths->_ZBufferParams.z *= -1;
		camera->UploadCameraBuffer(ths->passConstants);
		//Calculate Jitter Matrix
		Matrix4 nonJitterVP = transData->nonJitteredVPMatrix;
		ths->passConstants.nonJitterVP = nonJitterVP;
		ths->passConstants._FlipNonJitterVP = transData->nonJitteredFlipVP;
		ths->passConstants._FlipNonJitterInverseVP = inverse(transData->nonJitteredFlipVP);
		ths->passConstants._FlipLastVP = transData->lastFlipNonJitterVP;
		ths->passConstants._FlipInverseLastVP = inverse(transData->lastFlipNonJitterVP);
		ths->passConstants.nonJitterInverseVP = inverse(nonJitterVP);
		memcpy(&ths->passConstants.lastVP, &transData->lastNonJitterVP, sizeof(XMFLOAT4X4));
		ths->passConstants.lastInverseVP = inverse(transData->lastNonJitterVP);
		ths->passConstants._RandomSeed = {
			(float)randomComp.GetRangedFloat(1e-4, 9.99999),
			(float)randomComp.GetRangedFloat(1e-4, 9.99999),
			(float)randomComp.GetRangedFloat(1e-4, 9.99999),
			(float)randomComp.GetRangedFloat(1e-4, 9.99999)
		};

		ths->passConstants._ZBufferParams = ths->_ZBufferParams;
		ConstBufferElement ele = resource->cameraCBs[camera->GetInstanceID()];
		ele.buffer->CopyData(ele.element, &ths->passConstants);
		//Calculate Frustum Planes
		Vector3 cameraRight = camera->GetRight();
		Vector3 cameraUp = camera->GetUp();
		Vector3 cameraForward = camera->GetLook();
		Vector3 cameraPosition = camera->GetPosition();
		MathLib::GetPerspFrustumPlanes(cameraRight, cameraUp, cameraForward, cameraPosition, camera->GetFovY(), camera->GetAspect(), camera->GetNearZ(), camera->GetFarZ(), ths->frustumPlanes);
		//Calculate Frustum Bounding
		MathLib::GetFrustumBoundingBox(
			cameraRight, cameraUp, cameraForward, cameraPosition,
			camera->GetNearWindowHeight(),
			camera->GetFarWindowHeight(),
			camera->GetAspect(),
			camera->GetNearZ(),
			camera->GetFarZ(),
			&ths->frustumMinPos,
			&ths->frustumMaxPos
		);
	}
};

void PrepareComponent::RenderEvent(const EventData& data, ThreadCommand* commandList)
{
	PrepareRunnable runnable =
	{
		data.device,
		commandList,
		this,
		data.resource,
		data.camera,
		data.width, data.height,
		data.worldMovingDir,
		data.isMovingTheWorld
	};
	ScheduleJob(runnable);
}