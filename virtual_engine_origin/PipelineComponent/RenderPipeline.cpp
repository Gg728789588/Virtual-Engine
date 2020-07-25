#include "RenderPipeline.h"
#include "../JobSystem/JobSystem.h"
#include "CommandBuffer.h"
#include "PipelineComponent.h"
#include "../Common/Camera.h"
#include "../PipelineComponent/ThreadCommand.h"
#include "PrepareComponent.h"
#include "../LogicComponent/World.h"
#include "GeometryComponent.h"
#include "PostProcessingComponent.h"
#include "../RenderComponent/RenderCommand.h"
#include "LightingComponent.h"
#include "DepthComponent.h"
#include "../Singleton/PSOContainer.h"
#include "OpaquePostProcess.h"
#include "CSMComponent.h"
#include "../Common/Input.h"
#include "VolumetricComponent.h"
#include "../RenderComponent/Light.h"
#include "LocalShadowComponent.h"
#include "../LogicComponent/Transform.h"
#include "ReflectionComponent.h"
#include "../CJsonObject/CJsonObject.hpp"

using namespace Math;
RenderPipeline* RenderPipeline::current(nullptr);
ThreadCommand* InitThreadCommand(ID3D12Device* device, Camera* cam, FrameResource* resource, PipelineComponent* comp)
{
	switch (comp->GetCommandListType())
	{
	case CommandListType_Graphics:
		return resource->GetNewThreadCommand(cam, device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		break;
	case CommandListType_Compute:
		return resource->GetNewThreadCommand(cam, device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	case CommandListType_Copy:
		return resource->GetNewThreadCommand(cam, device, D3D12_COMMAND_LIST_TYPE_COPY);
	default:
		return nullptr;
	}
}
void ExecuteThreadCommand(Camera* cam, ThreadCommand* command, FrameResource* resource, PipelineComponent* comp)
{
	const D3D12_COMMAND_LIST_TYPE types[4] =
	{
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		D3D12_COMMAND_LIST_TYPE_COPY
	};
	if (command != nullptr) {
		resource->ReleaseThreadCommand(cam, command, types[comp->GetCommandListType()]);
	}
}

void RenderPipeline::mInit(PipelineComponent* ptr, const std::type_info& tp)
{
	components.push_back(ptr);
	componentsLink.Insert(tp, ptr);
}
void RenderPipeline::AddCameraRenderCommand(Camera* cam)
{
	std::lock_guard<std::mutex> lck(current->renderListMtx);
	current->manualRenderList.push_back(cam);
}

RenderPipeline::RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) : renderPathComponents(3)
{
	fenceCounter.reserve(20);
	manualRenderList.reserve(24);
	current = this;
	//Init All Events Here
	Init<
		PrepareComponent,

		//Compute Queues
		LightingComponent,
		VolumetricComponent,
		//TODO Copy Queue
		//Direct Queue
		CSMComponent,
		LocalShadowComponent,
		DepthComponent,
		GeometryComponent,
		ReflectionComponent,
		OpaquePostProcess,
		PostProcessingComponent
	>();

	for (UINT i = 0, size = components.size(); i < size; ++i)
	{
		PipelineComponent* comp = components[i];
		comp->Initialize(device, commandList);
		for (auto ite = comp->gpuDepending.begin(); ite != comp->gpuDepending.end(); ++ite)
		{
			(*ite)->CreateFence(device);
		}
	}
	//Init Path
	for (auto i = components.begin(); i != components.end(); ++i)
	{
		renderPathComponents[0].push_back(*i);
	}
}

void RenderPipeline::PrepareRendering(RenderPipelineData& renderData, JobSystem* jobSys,
	ArrayList <JobBucket*>& bucketArray) noexcept
{
	CommandBuffer& currentCommandBuffer = renderData.resource->commandBuffer;
	UINT camSize = Max<size_t>(renderData.allCameras->size(), 1);
	UINT bucketCountBeforeRendering = bucketArray.size();
	bucketArray.push_back(jobSys->GetJobBucket());
	PipelineComponent::EventData data;
	data.worldMovingDir = renderData.worldMovingDir;
	data.isMovingTheWorld = renderData.isMovingTheWorld;
	data.device = renderData.device;
	data.resource = renderData.resource;
	data.world = renderData.world;
	data.time = renderData.time;
	data.ringFrameIndex = renderData.ringFrameIndex;
	data.deltaTime = renderData.deltaTime;
	data.frameCount = *renderData.fenceIndex;

	ThreadCommand* directCommandList = renderData.resource->commmonThreadCommand;
	ThreadCommand* copyCommandList = renderData.resource->copyThreadCommand;
	JobBucket* bucket = bucketArray[bucketCountBeforeRendering];
	bucket->GetTask(nullptr, 0, [=]()->void
		{
			directCommandList->ResetCommand();
			copyCommandList->ResetCommand();

			while (RenderCommand::ExecuteCommand(
				data.device, directCommandList->GetCmdList(), copyCommandList->GetCmdList(), data.resource, directCommandList->GetBarrierBuffer()))
			{
			}
			directCommandList->CloseCommand();
			copyCommandList->CloseCommand();
		});
	//Execute & Signal Async Command
	currentCommandBuffer.Execute(renderData.asyncCopyQueue, copyCommandList->GetCmdList());
	currentCommandBuffer.Signal(renderData.asyncCopyQueue, renderData.copyToDirectFence, data.frameCount);
	//Move World Check
	if (renderData.isMovingTheWorld)
	{
		World* world = World::GetInstance();
		Vector3 worldMovingData = renderData.worldMovingDir;
		if (world)
		{

			/*	GRPRenderManager* renderManager = world->GetGRPRenderManager();
				if (renderManager)
				{
					renderManager->MoveTheWorld(renderData.device, beforeRenderingBucket, worldMovingData);
				}*/

		}
	}
	JobHandle fenceHandle;

	auto renderCameraFunc = [&](Camera* cam, uint64* rendererFrame)->void
	{
		if (*rendererFrame >= *renderData.fenceIndex) return;
		*rendererFrame = *renderData.fenceIndex;
		if (cam->renderTarget)
		{
			data.width = cam->renderTarget->GetWidth();
			data.height = cam->renderTarget->GetHeight();
			data.isBackBufferForPresent = false;
			data.backBufferHandle = cam->renderTarget->GetColorDescriptor(0, 0);
			data.backBuffer = cam->renderTarget->GetResource();
		}
		else
		{
			data.width = data.world->windowWidth;
			data.height = data.world->windowHeight;
			data.isBackBufferForPresent = true;
			data.backBuffer = renderData.backBufferResource;
			data.backBufferHandle = renderData.backBufferHandle;
		}
		data.camera = cam;
		ArrayList<PipelineComponent*>& waitingComponents = renderPathComponents[(UINT)cam->GetRenderingPath()];
		renderTextureMarks.Clear();
		//Read Events' Markers
		for (UINT i = 0, size = waitingComponents.size(); i < size; ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			ArrayList<TemporalResourceCommand>& descriptors = component->SendRenderTextureRequire(data);
			//Allocate Temporal Render Texture
			component->allTempResource.resize(descriptors.size());
			for (UINT j = 0, descriptorSize = descriptors.size(); j < descriptorSize; ++j)
			{
				TemporalResourceCommand& command = descriptors[j];
				if (command.type == TemporalResourceCommand::CommandType_Create_RenderTexture ||
					command.type == TemporalResourceCommand::CommandType_Create_StructuredBuffer)
				{
#ifdef _DEBUG
					//Alread contained
					if (renderTextureMarks.Contains(command.uID))
					{
						throw "Already Created Render Texture!";
					}
#endif
					RenderTextureMark mark = { command.uID, j, command.descriptor, i, i };
					renderTextureMarks.Insert(command.uID, mark);
				}
				else
				{
					RenderTextureMark& markPtr = renderTextureMarks[command.uID];
#ifdef _DEBUG
					if (&markPtr == nullptr)
					{
						throw "No Such Render Texture!";
					}
#endif
					markPtr.endComponent = i;
					//command.tempResState
					component->requiredRTs.push_back({
						j,
						command.uID });
				}
			}

		}
		//Mark Resources throw Markers
		renderTextureMarks.IterateAll([&](uint i, uint key, RenderTextureMark& mark)->void
			{
				waitingComponents[mark.startComponent]->loadRTCommands.push_back({ mark.id, mark.rtIndex, mark.desc });
				waitingComponents[mark.endComponent]->unLoadRTCommands.push_back(mark.id);
			});

		PipelineComponent::bucket = bucket;

		//Execute Events
		for (auto componentIte = waitingComponents.begin(); componentIte != waitingComponents.end(); ++componentIte)
		{
			PipelineComponent* component = *componentIte;
			component->threadCommand = InitThreadCommand(renderData.device, cam, renderData.resource, component);
			component->ExecuteTempRTCommand(renderData.device, &tempRTAllocator);
			component->ClearHandles();
			component->MarkDepending(fenceHandle);
			component->RenderEvent(data, component->threadCommand);
			{
				ID3D12CommandQueue* targetQueue = nullptr;
				switch (component->GetCommandListType())
				{
				case CommandListType_Graphics:
					targetQueue = renderData.directQueue;
					break;
				case CommandListType_Compute:
					targetQueue = renderData.computeQueue;
					break;
				case CommandListType_Copy:
					targetQueue = renderData.copyQueue;
					break;
				}
				if (targetQueue)
				{
					for (auto ite = component->gpuDepending.begin(); ite != component->gpuDepending.end(); ++ite)
					{
						currentCommandBuffer.Wait(targetQueue, (*ite)->fence.Get(), data.frameCount);
					}
					currentCommandBuffer.Execute(targetQueue, component->threadCommand->GetCmdList());
					if (component->dependingComponentCount > 0)
					{
						currentCommandBuffer.Signal(targetQueue, component->fence.Get(), data.frameCount);
					}
				}
			}
		}
		fenceCounter.clear();
		for (UINT i = 0, size = waitingComponents.size(); i < size; ++i)
		{
			PipelineComponent* component = waitingComponents[i];
			ExecuteThreadCommand(cam, component->threadCommand, renderData.resource, component);
			fenceCounter.push_back_all(component->jobHandles.data(), component->jobHandles.size());
		}
		AfterRenderEvent evt = { cam };
		fenceHandle = bucket->GetTask(fenceCounter.data(), fenceCounter.size(), [=]()->void
			{
				evt();
			});
	};
	//Execute All Camera
	for (UINT camIndex = 0; camIndex < renderData.allCameras->size(); ++camIndex)
	{
		Camera* cam = (*renderData.allCameras)[camIndex];
		//Stop Render Here
		if (!cam->autoRender) continue;
		renderCameraFunc(cam, &cam->renderedFrame);
	}
	Camera** manualCams = nullptr;
	uint64 camCount = 0;
	{
		std::lock_guard<std::mutex> lck(renderListMtx);
		if (!manualRenderList.empty())
		{
			camCount = manualRenderList.size();
			manualCams = (Camera**)alloca(sizeof(Camera*) * camCount);
			memcpy(manualCams, manualRenderList.data(), sizeof(Camera*) * camCount);
			manualRenderList.clear();
		}
	}
	for (uint64 i = 0; i < camCount; ++i)
	{
		Camera* cam = manualCams[i];
		renderCameraFunc(cam, &cam->renderedFrame);
	}
	//Wait For Copy Queue And Execute Post Direct Queue
	currentCommandBuffer.Wait(renderData.directQueue, renderData.copyToDirectFence, data.frameCount);
	currentCommandBuffer.Execute(renderData.directQueue, directCommandList->GetCmdList());
}

HRESULT RenderPipeline::ExecuteRendering(RenderPipelineData& renderData, IDXGISwapChain* swapChain) noexcept
{
	HRESULT hr = S_OK;
	if (renderData.lastResource)				//Run Last frame's commandqueue
	{
		renderData.lastResource->commandBuffer.Submit();
		renderData.lastResource->commandBuffer.Clear();
	}
	hr = swapChain->Present(0, 0);
	if (renderData.lastResource)
	{
		renderData.lastResource->UpdateAfterFrame(
			*renderData.fenceIndex,
			{
				std::pair<ID3D12CommandQueue*, ID3D12Fence*>(
					renderData.directQueue,
					renderData.fence),
				std::pair<ID3D12CommandQueue*, ID3D12Fence*>(
					renderData.copyQueue,
					renderData.copyFence)
			});//Signal CommandQueue
	}
	tempRTAllocator.CumulateReleaseAfterFrame();

	return hr;
}

RenderPipeline::~RenderPipeline()
{
	for (UINT i = 0; i < components.size(); ++i)
	{
		components[i]->Dispose();
		delete components[i];
	}
}

RenderPipeline* RenderPipeline::GetInstance(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	if (current == nullptr)
	{
		current = new RenderPipeline(device, commandList);
	}
	return current;
}

void RenderPipeline::DestroyInstance()
{
	if (current != nullptr)
	{
		auto ptr = current;
		current = nullptr;
		delete ptr;
	}
}
void RenderPipeline::AfterRenderEvent::operator()() const noexcept
{
	cam->ExecuteAllCallBack();
}