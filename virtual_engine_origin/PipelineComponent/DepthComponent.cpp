#include "DepthComponent.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../RenderComponent/RenderTexture.h"
#include "../LogicComponent/World.h"
#include "PrepareComponent.h"
#include "../Singleton/PSOContainer.h"
#include "RenderPipeline.h"
#include "../Singleton/Graphics.h"
#include "../RenderComponent/ComputeShader.h"
#include "../Common/Camera.h"
#include "../Utility/PassConstants.h"
#include "../Common/RenderPackage.h"

using namespace DirectX;
namespace DepthGlobal
{
	PrepareComponent* prepareComp_Depth = nullptr;
	const ComputeShader* hizGenerator;
	const uint2 hizResolution = { 1024, 512 };
	uint _CameraDepthTexture;
	uint _DepthTexture;
}
using namespace DepthGlobal;

void DepthComponent::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	SetCPUDepending<PrepareComponent>();
	tempRTRequire.resize(2);
	tempRTRequire.SetZero();
	TemporalResourceCommand& depthBuffer = tempRTRequire[0];
	depthBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	depthBuffer.uID = ShaderID::PropertyToID("_CameraDepthTexture");
	depthBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::DepthBuffer;
	depthBuffer.descriptor.rtDesc.rtFormat.depthFormat = RenderTextureDepthSettings_Depth32;
	depthBuffer.descriptor.rtDesc.depthSlice = 1;
	depthBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;
	TemporalResourceCommand& motionVectorBuffer = tempRTRequire[1];
	motionVectorBuffer.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
	motionVectorBuffer.uID = ShaderID::PropertyToID("_CameraMotionVectorsTexture");
	motionVectorBuffer.descriptor.rtDesc.rtFormat.colorFormat = DXGI_FORMAT_R16G16_SNORM;
	motionVectorBuffer.descriptor.rtDesc.rtFormat.usage = RenderTextureUsage::ColorBuffer;
	motionVectorBuffer.descriptor.rtDesc.depthSlice = 1;
	motionVectorBuffer.descriptor.rtDesc.type = TextureDimension::Tex2D;
	prepareComp_Depth = RenderPipeline::GetComponent<PrepareComponent>();
	depthPrepassContainer = new PSOContainer();
	_DepthTexture = ShaderID::PropertyToID("_DepthTexture");
	_CameraDepthTexture = ShaderID::PropertyToID("_CameraDepthTexture");
}
void DepthComponent::Dispose()
{
	delete depthPrepassContainer;
}
ArrayList<TemporalResourceCommand>& DepthComponent::SendRenderTextureRequire(const EventData& evt)
{
	for (auto ite = tempRTRequire.begin(); ite != tempRTRequire.end(); ++ite)
	{
		ite->descriptor.rtDesc.width = evt.width;
		ite->descriptor.rtDesc.height = evt.height;
	}
	return tempRTRequire;
}

class DepthRunnable
{
public:
	RenderTexture* depthRT;
	RenderTexture* motionVecRT;
	ID3D12Device* device;		//Dx12 Device
	ThreadCommand* tcmd;		//Command List
	Camera* cam;				//Camera
	FrameResource* resource;	//Per Frame Data
	DepthComponent* component;//Singleton Component
	World* world;				//Main Scene
	uint frameIndex;
	void operator()()
	{
		ThreadCommandFollower follower(tcmd);
		TransitionBarrierBuffer* barrierBuffer = tcmd->GetBarrierBuffer();
		ID3D12GraphicsCommandList* commandList = tcmd->GetCmdList();
		
		depthRT->ClearRenderTarget(commandList, 0, 0);
		RenderPackage package(
			device,
			commandList,
			resource,
			barrierBuffer,
			component->depthPrepassContainer);
	
		//Draw Depth prepass
		component->depthPrepassContainer->SetRenderTarget(commandList, {}, depthRT);

	}
};

void DepthComponent::RenderEvent(const EventData& data, ThreadCommand* commandList)
{
	ScheduleJob<DepthRunnable>({
		(RenderTexture*)allTempResource[0],
		(RenderTexture*)allTempResource[1],
		data.device,
		commandList,
		data.camera,
		data.resource,
		this,
		data.world,
		data.ringFrameIndex
		});
}