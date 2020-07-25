#include "ReflectionComponent.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/PSOContainer.h"
#include "../RenderComponent/RenderComponentInclude.h"
#include "PrepareComponent.h";
#include "GeometryComponent.h"
#include "ScreenSpaceReflection.h"
#include "../LogicComponent/World.h"
using namespace Math;
PrepareComponent* prepareComp_ReflectionEvt;
GeometryComponent* skyboxComp_ReflectionEvt;
StackObject<PSOContainer> psoContainer_Reflection;
const Shader* reflectionShader_Reflection;
StackObject<ScreenSpaceReflection> ssrReflComp_ReflComp;
#define prepareComp prepareComp_ReflectionEvt
#define reflectionShader reflectionShader_Reflection
#define psoContainer psoContainer_Reflection
#define skyboxComp skyboxComp_ReflectionEvt
#define ssrReflComp ssrReflComp_ReflComp

ArrayList<TemporalResourceCommand>& ReflectionComponent::SendRenderTextureRequire(const EventData& evt)
{
	ssrReflComp->UpdateTempRT(evt, tempRT);
	tempRT[6].descriptor.rtDesc.width = evt.width;
	tempRT[6].descriptor.rtDesc.height = evt.height;
	tempRT[7].descriptor.rtDesc.width = evt.width;
	tempRT[7].descriptor.rtDesc.height = evt.height;
	return tempRT;
}

struct ReflectionRunnable
{
	RenderTexture* gbuffer0Tex;
	RenderTexture* gbuffer1Tex;
	RenderTexture* gbuffer2Tex;
	RenderTexture* mvTex;
	RenderTexture* depthTex;
	RenderTexture* emissionTex;
	RenderTexture* reflectionTex;
	ReflectionComponent* selfPtr;
	ThreadCommand* tCmd;
	FrameResource* resource;
	ID3D12Device* device;
	Camera* cam;
	void operator()()
	{
		ThreadCommandFollower follower(tCmd);
		auto commandList = tCmd->GetCmdList();
		TransitionBarrierBuffer* barrierBuffer = tCmd->GetBarrierBuffer();
#define ADD_READ_COMMAND(tex) barrierBuffer->AddCommand(tex##->GetWriteState(), tex##->GetReadState(), tex##->GetResource())
#define ADD_WRITE_COMMAND(tex) barrierBuffer->AddCommand(tex##->GetReadState(), tex##->GetWriteState(), tex##->GetResource())
		ADD_READ_COMMAND(gbuffer0Tex);
		ADD_READ_COMMAND(gbuffer1Tex);
		ADD_READ_COMMAND(gbuffer2Tex);
		ADD_READ_COMMAND(mvTex);
		ADD_READ_COMMAND(depthTex);

		barrierBuffer->ExecuteCommand(commandList);

		GBufferCameraData* gbufferCamData = (GBufferCameraData*)cam->GetResource(skyboxComp, [&]()->GBufferCameraData*
			{
				return new GBufferCameraData(device);
			});

		reflectionShader->BindRootSignature(commandList, Graphics::GetGlobalDescHeap());
		const Mesh* cubeMesh = Graphics::GetCubeMesh();
		ConstBufferElement cameraBuffer = resource->cameraCBs[cam->GetInstanceID()];
		reflectionShader->SetResource(commandList, ShaderID::GetPerCameraBufferID(), cameraBuffer.buffer, cameraBuffer.element);
		reflectionShader->SetResource(commandList, selfPtr->tempRT[4].uID, Graphics::GetGlobalDescHeap(), depthTex->GetGlobalDescIndex());
		reflectionShader->SetResource(commandList, selfPtr->_Cubemap, Graphics::GetGlobalDescHeap(), 0);
		reflectionShader->SetResource(commandList, selfPtr->_CameraGBuffer0, Graphics::GetGlobalDescHeap(), gbuffer0Tex->GetGlobalDescIndex());
		reflectionShader->SetResource(commandList, selfPtr->_CameraGBuffer1, Graphics::GetGlobalDescHeap(), gbuffer1Tex->GetGlobalDescIndex());
		reflectionShader->SetResource(commandList, selfPtr->_CameraGBuffer2, Graphics::GetGlobalDescHeap(), gbuffer2Tex->GetGlobalDescIndex());
		
		World* world = World::GetInstance();
		reflectionTex->ClearRenderTarget(commandList, 0, 0);
		if (world->currentSkybox)
		{

			reflectionShader->SetResource(commandList, selfPtr->TextureIndices, &gbufferCamData->texIndicesBuffer, 0);

			Graphics::Blit(
				commandList,
				device,
				{ RenderTarget(reflectionTex, 0, 0) },
				RenderTarget(depthTex, 0, 0),
				psoContainer,
				reflectionShader, 2);

			ADD_READ_COMMAND(emissionTex);
			//SSR
			ssrReflComp->FrameUpdate(
				device,
				commandList,
				resource,
				cam,
				barrierBuffer,
				depthTex,
				gbuffer1Tex,
				gbuffer2Tex,
				emissionTex,
				reflectionTex,
				mvTex,
				psoContainer);

			ADD_READ_COMMAND(reflectionTex);
			ADD_WRITE_COMMAND(emissionTex);
			barrierBuffer->ExecuteCommand(commandList);
			//Add To Final Data
			reflectionShader->BindRootSignature(commandList, Graphics::GetGlobalDescHeap());
			reflectionShader->SetResource(commandList, ShaderID::GetMainTex(), Graphics::GetGlobalDescHeap(), reflectionTex->GetGlobalDescIndex());
			Graphics::Blit(commandList,
				device,
				{ RenderTarget(emissionTex) },
				RenderTarget(depthTex),
				psoContainer,
				reflectionShader, 1);
			//Calculate Reflection Here!
			ADD_WRITE_COMMAND(gbuffer0Tex);
			ADD_WRITE_COMMAND(gbuffer1Tex);
			ADD_WRITE_COMMAND(reflectionTex);
			ADD_WRITE_COMMAND(gbuffer2Tex);

		}
	};
};
void ReflectionComponent::RenderEvent(const EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<ReflectionRunnable>({
			  (RenderTexture*)allTempResource[0],
			  (RenderTexture*)allTempResource[1],
			  (RenderTexture*)allTempResource[2],
			  (RenderTexture*)allTempResource[3],
			  (RenderTexture*)allTempResource[4],
			  (RenderTexture*)allTempResource[5],
			  (RenderTexture*)allTempResource[6],
			  this,
			  commandList,
			  data.resource,
			  data.device,
			  data.camera
		});
	ssrReflComp->PrepareRenderTexture(allTempResource);
}

void ReflectionComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* command)
{
	SetCPUDepending<PrepareComponent>();
	prepareComp = RenderPipeline::GetComponent<PrepareComponent>();
	skyboxComp = RenderPipeline::GetComponent<GeometryComponent>();
	tempRT.resize(7);
	tempRT.SetZero();
	for (auto ite = tempRT.begin(); ite != tempRT.end(); ++ite)
		ite->type = TemporalResourceCommand::CommandType_Require_RenderTexture;
	tempRT[0].uID = ShaderID::PropertyToID("_CameraGBufferTexture0");
	tempRT[1].uID = ShaderID::PropertyToID("_CameraGBufferTexture1");
	tempRT[2].uID = ShaderID::PropertyToID("_CameraGBufferTexture2");
	tempRT[3].uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	tempRT[4].uID = ShaderID::PropertyToID("_CameraDepthTexture");
	tempRT[5].uID = ShaderID::PropertyToID("_CameraRenderTarget");
	TemporalResourceCommand& emissionBuffer = tempRT[6];
	emissionBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	emissionBuffer.uID = ShaderID::PropertyToID("_ReflectionTexture");
	emissionBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	emissionBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::ColorBuffer;
	emissionBuffer.descriptor.rtDesc.depthSlice = 1;
	emissionBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;

	psoContainer.New();
	reflectionShader = ShaderCompiler::GetShader("Reflection");
	TextureIndices = ShaderID::PropertyToID("TextureIndices");
	_CameraGBuffer0 = ShaderID::PropertyToID("_CameraGBuffer0");
	_CameraGBuffer1 = ShaderID::PropertyToID("_CameraGBuffer1");
	_CameraGBuffer2 = ShaderID::PropertyToID("_CameraGBuffer2");
	_Cubemap = ShaderID::PropertyToID("_Cubemap");
	ssrReflComp.New(device, command, tempRT, ShaderCompiler::GetShader("SSRBlit"));
}

void ReflectionComponent::Dispose()
{
	psoContainer.Delete();
	ssrReflComp.Delete();
}

#undef reflectionShader
#undef prepareComp
#undef psoContainer
#undef ADD_READ_COMMAND
#undef ADD_WRITE_COMMAND
#undef skyboxComp
#undef ssrReflComp