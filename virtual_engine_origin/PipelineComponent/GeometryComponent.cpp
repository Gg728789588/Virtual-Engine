#include "GeometryComponent.h"
#include "../Singleton/ShaderID.h"
#include "../RenderComponent/Skybox.h"
#include "../Singleton/PSOContainer.h"
#include "RenderPipeline.h"
#include "../RenderComponent/Texture.h"
#include "../RenderComponent/MeshRenderer.h"
#include "LightingComponent.h";
#include "../Singleton/ShaderCompiler.h"
#include "../LogicComponent/World.h"
#include "DepthComponent.h"
#include "PrepareComponent.h"
#include "CameraData/CameraTransformData.h"
#include "../Common/Camera.h"

//#include "BaseColorComponent.h"
uint _DefaultMaterials;
StackObject<PSOContainer> gbufferContainer;

DepthComponent* depthComp;
const Shader* terrainTestShader;
using namespace Math;
PrepareComponent* prepareComp_Skybox;
struct TextureIndices
{
	uint _SkyboxCubemap;
	uint _PreintTexture;
};
const uint TEX_COUNT = sizeof(TextureIndices) / 4;

LightingComponent* lightComp_GBufferGlobal;
uint GeometryComponent::_AllLight(0);
uint GeometryComponent::_LightIndexBuffer(0);
uint GeometryComponent::LightCullCBuffer(0);
uint GeometryComponent::TextureIndices(0);
uint GeometryComponent::_GreyTex;
uint GeometryComponent::_IntegerTex;
uint GeometryComponent::_Cubemap;
uint GeometryComponent::_GreyCubemap;
uint GeometryComponent::_TextureIndirectBuffer;
uint GeometryComponent::DecalParams;
uint GeometryComponent::_IntegerTex3D;
uint GeometryComponent::_AllDecal;

struct SkyboxBuffer
{
	XMFLOAT4X4 invvp;
	XMFLOAT4X4 nonJitterVP;
	XMFLOAT4X4 lastVP;
};


GBufferCameraData::GBufferCameraData(ID3D12Device* device) :
	texIndicesBuffer(
		device, 1, true, sizeof(TextureIndices)
	),
	posBuffer(device,
		3, true,
		sizeof(SkyboxBuffer))
{
}

GBufferCameraData::~GBufferCameraData()
{

}
XMFLOAT4X4 CalculateViewMatrix(Camera* cam)
{
	Vector3 R = cam->GetRight();
	Vector3 U = cam->GetUp();
	Vector3 L = cam->GetLook();
	// Keep camera's axes orthogonal to each other and of unit length.
	L = normalize(L);
	U = normalize(cross(L, R));

	// U, L already ortho-normal, so no need to normalize cross product.
	R = cross(U, L);

	XMFLOAT3* mRight = (XMFLOAT3*)&R;
	XMFLOAT3* mUp = (XMFLOAT3*)&U;
	XMFLOAT3* mLook = (XMFLOAT3*)&L;
	XMFLOAT4X4 mView;
	mView(0, 0) = mRight->x;
	mView(1, 0) = mRight->y;
	mView(2, 0) = mRight->z;
	mView(3, 0) = 0;

	mView(0, 1) = mUp->x;
	mView(1, 1) = mUp->y;
	mView(2, 1) = mUp->z;
	mView(3, 1) = 0;

	mView(0, 2) = mLook->x;
	mView(1, 2) = mLook->y;
	mView(2, 2) = mLook->z;
	mView(3, 2) = 0;

	mView(0, 3) = 0.0f;
	mView(1, 3) = 0.0f;
	mView(2, 3) = 0.0f;
	mView(3, 3) = 1.0f;
	return mView;
}

class SkyboxRunnable
{
public:
	RenderTexture* emissionTex;
	RenderTexture* mvTex;
	RenderTexture* depthTex;
	RenderTexture* gbuffer0Tex;
	RenderTexture* gbuffer1Tex;
	RenderTexture* gbuffer2Tex;
	GeometryComponent* selfPtr;
	ThreadCommand* commandList;
	FrameResource* resource;
	ID3D12Device* device;
	Camera* cam;
	uint64 frameCount;
	uint frameIndex;
	uint width;
	uint height;
	bool isBackBufferRendering;


	void RenderGBuffer(GBufferCameraData* camData, const RenderPackage& pack, TransitionBarrierBuffer* barrierBuffer)
	{

		auto commandList = this->commandList->GetCmdList();
		auto world = World::GetInstance();
		UploadBuffer* allDecals = nullptr;

		TextureIndices* indices = (TextureIndices*)camData->texIndicesBuffer.GetMappedDataPtr(0);
		//Set
		if (world->currentSkybox)
		{
			indices->_SkyboxCubemap = world->currentSkybox->GetTexture()->GetGlobalDescIndex();
		}
		indices->_PreintTexture = selfPtr->preintTexture->GetGlobalDescIndex();

		LightFrameData* lightFrameData = (LightFrameData*)resource->GetPerCameraResource(lightComp_GBufferGlobal, cam, []()->LightFrameData*
			{
#ifndef NDEBUG
				throw "No Light Data Exception!";
#endif
				return nullptr;	//Get Error if there is no light coponent in pipeline
			});
		LightCameraData* lightCameraData = (LightCameraData*)cam->GetResource(lightComp_GBufferGlobal, []()->LightCameraData*
			{
#ifndef NDEBUG
				throw "No Light Data Exception!";
#endif
				return nullptr;	//Get Error if there is no light coponent in pipeline
			});
		const DescriptorHeap* worldHeap = Graphics::GetGlobalDescHeap();

		const Shader* gbufferShader = nullptr;//world->GetGRPRenderManager()->GetShader();

		auto setGBufferShaderFunc = [&]()->void
		{
			gbufferShader->BindRootSignature(commandList, worldHeap);
			gbufferShader->SetResource(commandList, ShaderID::GetMainTex(), worldHeap, 0);
			gbufferShader->SetResource(commandList, GeometryComponent::_GreyTex, worldHeap, 0);
			gbufferShader->SetResource(commandList, GeometryComponent::_IntegerTex, worldHeap, 0);
			gbufferShader->SetResource(commandList, GeometryComponent::_Cubemap, worldHeap, 0);
			gbufferShader->SetResource(commandList, GeometryComponent::_GreyCubemap, worldHeap, 0);
			gbufferShader->SetStructuredBufferByAddress(commandList, GeometryComponent::_AllLight, lightFrameData->lightsInFrustum.GetAddress(0));
			gbufferShader->SetStructuredBufferByAddress(commandList, GeometryComponent::_LightIndexBuffer, lightComp_GBufferGlobal->lightIndexBuffer->GetAddress(0, 0));
			gbufferShader->SetResource(commandList, GeometryComponent::LightCullCBuffer, &lightCameraData->lightCBuffer, frameIndex);
			gbufferShader->SetResource(commandList, GeometryComponent::TextureIndices, &camData->texIndicesBuffer, 0);
			gbufferShader->SetResource(commandList, GeometryComponent::_AllDecal, allDecals, 0);
			gbufferShader->SetResource(commandList, GeometryComponent::_IntegerTex3D, worldHeap, 0);
			auto cb = resource->cameraCBs[cam->GetInstanceID()];
			gbufferShader->SetResource(commandList, ShaderID::GetPerCameraBufferID(),
				cb.buffer, cb.element);
		};

		//setGBufferShaderFunc();
		barrierBuffer->ExecuteCommand(commandList);

		const RenderTexture* rts[6] =
		{
			gbuffer0Tex , gbuffer1Tex, gbuffer2Tex, mvTex, emissionTex
		};
		uint rtCount = 5;
#pragma region Clear_RenderState
		gbuffer0Tex->ClearRenderTarget(commandList, 0, 0);
		gbuffer1Tex->ClearRenderTarget(commandList, 0, 0);
		gbuffer2Tex->ClearRenderTarget(commandList, 0, 0);
		mvTex->ClearRenderTarget(commandList, 0, 0);
		emissionTex->ClearRenderTarget(commandList, 0, 0);
#pragma endregion


#pragma region Draw_First_Geometry
		gbufferContainer->SetRenderTarget(
			commandList,
			rts,
			rtCount,
			depthTex);
#pragma endregion

	}

	void operator()()
	{
		ThreadCommandFollower follower(commandList);
		TransitionBarrierBuffer* barrierBuffer = commandList->GetBarrierBuffer();
		barrierBuffer->RegistInitState(depthTex->GetWriteState(), depthTex->GetResource());
		auto world = World::GetInstance();
		ID3D12GraphicsCommandList* cmdList = commandList->GetCmdList();
		RenderPackage pack(
			device,
			commandList->GetCmdList(),
			resource,
			this->commandList->GetBarrierBuffer(),
			gbufferContainer);
		if (!selfPtr->preintTexture)
		{
			RenderTextureFormat format;
			format.colorFormat = DXGI_FORMAT_R16G16_UNORM;

			format.usage = RenderTextureUsage::ColorBuffer;
			selfPtr->preintTexture = new RenderTexture(
				device,
				256,
				256,
				format,
				TextureDimension::Tex2D,
				1,
				0);
			const Shader* preintShader = ShaderCompiler::GetShader("PreInt");
			preintShader->BindRootSignature(cmdList);
			Graphics::Blit(
				cmdList,
				device,
				{ RenderTarget(selfPtr->preintTexture, 0, 0) },
				RenderTarget(),
				gbufferContainer,
				preintShader,
				0
			);
			barrierBuffer->AddCommand(selfPtr->preintTexture->GetWriteState(), selfPtr->preintTexture->GetReadState(), selfPtr->preintTexture->GetResource());
		}
		GBufferCameraData* camData = (GBufferCameraData*)cam->GetResource(selfPtr, [&]()->GBufferCameraData*
			{
				return new GBufferCameraData(device);
			});

		RenderGBuffer(camData, pack, barrierBuffer);
		if (world->currentSkybox)
		{
			CameraTransformData* transData = (CameraTransformData*)cam->GetResource(prepareComp_Skybox, []()->CameraTransformData*
				{
					return new CameraTransformData;
				});
			Matrix4 view = CalculateViewMatrix(cam);
			Matrix4 viewProj = mul(view, cam->GetProj());
			SkyboxBuffer bf;
			bf.invvp = inverse(viewProj);
			bf.nonJitterVP = mul(view, transData->nonJitteredProjMatrix);
			bf.lastVP = camData->vpMatrix;
			camData->vpMatrix = bf.nonJitterVP;
			camData->posBuffer.CopyData(frameIndex, &bf);
			gbufferContainer->SetRenderTarget(cmdList, { emissionTex, mvTex }, depthTex);
			ConstBufferElement skyboxData(&camData->posBuffer, frameIndex);
			world->currentSkybox->Draw(
				0,
				&skyboxData,
				pack
			);
		}
	}
};

ArrayList<TemporalResourceCommand>& GeometryComponent::SendRenderTextureRequire(const EventData& evt)
{
	for (int i = 0; i < tempRT.size(); ++i)
	{
		tempRT[i].descriptor.rtDesc.width = evt.width;
		tempRT[i].descriptor.rtDesc.height = evt.height;
	}

	return tempRT;
}
void GeometryComponent::RenderEvent(const EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<SkyboxRunnable>(
		{
			 (RenderTexture*)allTempResource[0],
			  (RenderTexture*)allTempResource[1],
			  (RenderTexture*)allTempResource[2],
			  (RenderTexture*)allTempResource[3],
			  (RenderTexture*)allTempResource[4],
			  (RenderTexture*)allTempResource[5],
			 this,
			 commandList,
			 data.resource,
			 data.device,
			 data.camera,
			 data.frameCount,
			 data.ringFrameIndex,
			 data.width,
			 data.height,
			 data.isBackBufferForPresent
		});
}


void GeometryComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<LightingComponent, PrepareComponent>();
	tempRT.resize(6);
	tempRT.SetZero();
	prepareComp_Skybox = RenderPipeline::GetComponent<PrepareComponent>();
	terrainTestShader = ShaderCompiler::GetShader("Terrain");
	lightComp_GBufferGlobal = RenderPipeline::GetComponent<LightingComponent>();
	depthComp = RenderPipeline::GetComponent<DepthComponent>();
	TemporalResourceCommand& emissionBuffer = tempRT[0];
	emissionBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	emissionBuffer.uID = ShaderID::PropertyToID("_CameraRenderTarget");
	emissionBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	emissionBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::ColorBuffer;
	emissionBuffer.descriptor.rtDesc.depthSlice = 1;
	emissionBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;
	tempRT[1].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[1].uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	tempRT[2].type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[2].uID = ShaderID::PropertyToID("_CameraDepthTexture");

	TemporalResourceCommand& specularBuffer = tempRT[3];
	specularBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	specularBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture0");
	specularBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	specularBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::ColorBuffer;
	specularBuffer.descriptor.rtDesc.depthSlice = 1;
	specularBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;
	TemporalResourceCommand& albedoBuffer = tempRT[4];
	albedoBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	albedoBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture1");
	albedoBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	albedoBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::ColorBuffer;
	albedoBuffer.descriptor.rtDesc.depthSlice = 1;
	albedoBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;
	TemporalResourceCommand& normalBuffer = tempRT[5];
	normalBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	normalBuffer.uID = ShaderID::PropertyToID("_CameraGBufferTexture2");
	normalBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	normalBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::ColorBuffer;
	normalBuffer.descriptor.rtDesc.depthSlice = 1;
	normalBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;
	/*
	PSORTSetting settings[3];
	settings[0].rtCount = 5;
	settings[0].rtFormat[0] = albedoBuffer.descriptor.rtDesc.rtFormat.colorFormat;
	settings[0].rtFormat[1] = specularBuffer.descriptor.rtDesc.rtFormat.colorFormat;
	settings[0].rtFormat[2] = normalBuffer.descriptor.rtDesc.rtFormat.colorFormat;
	settings[0].rtFormat[3] = DXGI_FORMAT_R16G16_SNORM;
	settings[0].rtFormat[4] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	settings[0].depthFormat = DXGI_FORMAT_D32_FLOAT;
	settings[1].rtCount = 0;
	settings[1].depthFormat = DXGI_FORMAT_D32_FLOAT;
	settings[2].depthFormat = DXGI_FORMAT_D32_FLOAT;
	settings[2].rtCount = 2;
	settings[2].rtFormat[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	settings[2].rtFormat[1] = DXGI_FORMAT_R16G16_SNORM;

	gbufferContainer.New(settings, 3);*/
	gbufferContainer.New();

	_AllLight = ShaderID::PropertyToID("_AllLight");
	_LightIndexBuffer = ShaderID::PropertyToID("_LightIndexBuffer");
	LightCullCBuffer = ShaderID::PropertyToID("LightCullCBuffer");
	TextureIndices = ShaderID::PropertyToID("TextureIndices");
	_DefaultMaterials = ShaderID::PropertyToID("_DefaultMaterials");
	_GreyTex = ShaderID::PropertyToID("_GreyTex");
	_IntegerTex = ShaderID::PropertyToID("_IntegerTex");
	_Cubemap = ShaderID::PropertyToID("_Cubemap");
	_GreyCubemap = ShaderID::PropertyToID("_GreyCubemap");
	_TextureIndirectBuffer = ShaderID::PropertyToID("_TextureIndirectBuffer");
	DecalParams = ShaderID::PropertyToID("DecalParams");
	_IntegerTex3D = ShaderID::PropertyToID("_IntegerTex3D");
	_AllDecal = ShaderID::PropertyToID("_AllDecal");
}
void GeometryComponent::Dispose()
{
	gbufferContainer.Delete();
}