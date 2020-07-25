#include "../PipelineComponent/CommandBuffer.h"
#include "FrameResource.h"
#include "../Utility/PassConstants.h"
#include "../Common/Camera.h"
vengine::vector<std::unique_ptr<FrameResource>> FrameResource::mFrameResources;
Pool<ThreadCommand> FrameResource::threadCommandMemoryPool(100);
Pool<FrameResource::FrameResCamera > FrameResource::perCameraDataMemPool(50);
vengine::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> FrameResource::needClearResourcesAfterFlush;
FrameResource* FrameResource::mCurrFrameResource = nullptr;
CBufferPool FrameResource::cameraCBufferPool(sizeof(PassConstants), 32);
vengine::vector<Runnable<void()>> FrameResource::needDisposeResourcesAfterFlush;
FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount) :
	perCameraDatas(23),
	cameraCBs(23),
	objectCBs(23)
{
	commmonThreadCommand = threadCommandMemoryPool.New(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	copyThreadCommand = threadCommandMemoryPool.New(device, D3D12_COMMAND_LIST_TYPE_COPY);
	unloadCommands.reserve(4);
	needClearResources.reserve(10);
	needClearResourcesAfterFlush.reserve(10);
}

void FrameResource::UpdateBeforeFrame(const std::initializer_list<ID3D12Fence*>& fences)
{
	if (Fence != 0)
	{
		for (auto ite = fences.begin(); ite != fences.end(); ++ite)
		{
			if ((*ite)->GetCompletedValue() < Fence)
			{
				LPCWSTR falseValue = 0;
				HANDLE eventHandle = CreateEventEx(nullptr, falseValue, false, EVENT_ALL_ACCESS);
				ThrowIfFailed((*ite)->SetEventOnCompletion(Fence, eventHandle));
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}
		}
		{
			std::lock_guard lck(mtx);
			needClearResources.clear();
			if (!needDisposeResources.empty())
			{
				for (auto ite = needDisposeResources.begin(); ite != needDisposeResources.end(); ++ite)
				{
					(*ite)();
				}
				needDisposeResources.clear();
			}
		}
		needClearResourcesAfterFlush.clear();
		if (!needDisposeResourcesAfterFlush.empty())
		{
			for (auto ite = needDisposeResourcesAfterFlush.begin(); ite != needDisposeResourcesAfterFlush.end(); ++ite)
			{
				(*ite)();
			}
			needDisposeResourcesAfterFlush.clear();
		}

		for (auto ite = unloadCommands.begin(); ite != unloadCommands.end(); ++ite)
		{
			uint32_t targetCamera = *ite;
			std::lock_guard<std::mutex> lck(mtx);
			FrameResCamera* data = perCameraDatas[targetCamera];
			for (auto ite = data->graphicsThreadCommands.begin(); ite != data->graphicsThreadCommands.end(); ++ite)
			{
				threadCommandMemoryPool.Delete(*ite);
			}
			for (auto ite = data->computeThreadCommands.begin(); ite != data->computeThreadCommands.end(); ++ite)
			{
				threadCommandMemoryPool.Delete(*ite);
			}
			perCameraDatas.Remove(targetCamera);
			ConstBufferElement& constBuffer = cameraCBs[targetCamera];
			cameraCBufferPool.Return(constBuffer);
			cameraCBs.Remove(targetCamera);
			perCameraDataMemPool.Delete(data);
		}
		unloadCommands.clear();
	}
}

void FrameResource::UpdateAfterFrame(UINT64& currentFence, const std::initializer_list<std::pair< ID3D12CommandQueue*, ID3D12Fence*>>& commandQueues)
{
	// Advance the fence value to mark commands up to this fence point.
	Fence = ++currentFence;

	for (auto ite = commandQueues.begin(); ite != commandQueues.end(); ++ite)
	{
		ite->first->Signal(ite->second, Fence);
	}
}

void FrameResource::OnLoadCamera(Camera* targetCamera, ID3D12Device* device)
{
	std::lock_guard<std::mutex> lck(mtx);
	perCameraDatas.Insert(targetCamera->GetInstanceID(), perCameraDataMemPool.New());
	ConstBufferElement constBuffer = cameraCBufferPool.Get(device);
	cameraCBs.Insert(targetCamera->GetInstanceID(), constBuffer);
}
void FrameResource::OnUnloadCamera(Camera* targetCamera)
{
	unloadCommands.push_back(targetCamera->GetInstanceID());
}

void FrameResource::ReleaseResourceAfterFlush(const Microsoft::WRL::ComPtr<ID3D12Resource>& resources)
{
	if (this == nullptr)
		needClearResourcesAfterFlush.push_back(resources);
	else
	{
		std::lock_guard lck(mtx);
		needClearResources.push_back(resources);
	}
}

void FrameResource::DisposeTargetAfterFlush(Runnable<void()> const& disposer)
{
	if (this == nullptr)
		needDisposeResourcesAfterFlush.push_back(disposer);
	else
	{
		std::lock_guard lck(mtx);
		needDisposeResources.push_back(disposer);
	}
}

ThreadCommand* FrameResource::GetNewThreadCommand(Camera* cam, ID3D12Device* device, D3D12_COMMAND_LIST_TYPE cmdListType)
{
	ArrayList<ThreadCommand*>* threadCommands = nullptr;
	switch (cmdListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		threadCommands = &perCameraDatas[cam->GetInstanceID()]->graphicsThreadCommands;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		threadCommands = &perCameraDatas[cam->GetInstanceID()]->computeThreadCommands;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		threadCommands = &perCameraDatas[cam->GetInstanceID()]->copyThreadCommands;
	}
	if (threadCommands->empty())
	{
		return threadCommandMemoryPool.New(device, cmdListType);
	}
	else
	{
		auto ite = threadCommands->end() - 1;
		ThreadCommand* result = *ite;
		threadCommands->erase(ite);
		return result;
	}
}
void FrameResource::ReleaseThreadCommand(Camera* cam, ThreadCommand* targetCmd, D3D12_COMMAND_LIST_TYPE cmdListType)
{
	ArrayList<ThreadCommand*>* threadCommands = nullptr;
	switch (cmdListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		threadCommands = &perCameraDatas[cam->GetInstanceID()]->graphicsThreadCommands;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		threadCommands = &perCameraDatas[cam->GetInstanceID()]->computeThreadCommands;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		threadCommands = &perCameraDatas[cam->GetInstanceID()]->copyThreadCommands;
	}
	threadCommands->push_back(targetCmd);
}

FrameResource::~FrameResource()
{
	if (mCurrFrameResource == this)
		mCurrFrameResource = nullptr;
	threadCommandMemoryPool.Delete(commmonThreadCommand);
	threadCommandMemoryPool.Delete(copyThreadCommand);
	/*for (auto ite = perFrameResource.Begin(); ite != perFrameResource.End(); ++ite)
	{
		delete ite.Value();
	}
	for (auto ite = perCameraDatas.Begin(); ite != perCameraDatas.End(); ++ite)
	{
		auto& camRes = ite.Value()->perCameraResource;
		for (auto secondIte = camRes.Begin(); secondIte != camRes.End(); ++secondIte)
		{
			delete secondIte.Value();
		}
	}*/
}

void FrameResource::DisposeResource(void* targetComponent)
{
	std::lock_guard<std::mutex> lck(mtx);
	auto ite = perFrameResource.Find(targetComponent);
	if (!ite) return;
	delete ite.Value();
	perFrameResource.Remove(ite);
}

void FrameResource::DisposePerCameraResource(void* targetComponent, Camera* cam)
{
	std::lock_guard<std::mutex> lck(mtx);
	FrameResCamera* ptr = perCameraDatas[cam->GetInstanceID()];
	auto ite = ptr->perCameraResource.Find(targetComponent);
	if (!ite) return;
	delete ite.Value();
	ptr->perCameraResource.Remove(ite);
}

void FrameResource::DisposeResources(void* targetComponent)
{
	for (auto ite = mFrameResources.begin(); ite != mFrameResources.end(); ++ite)
	{
		if (*ite) (*ite)->DisposeResource(targetComponent);
	}
}
void FrameResource::DisposePerCameraResources(void* targetComponent, Camera* cam)
{
	for (auto ite = mFrameResources.begin(); ite != mFrameResources.end(); ++ite)
	{
		if (*ite) (*ite)->DisposePerCameraResource(targetComponent, cam);
	}
}

IPipelineResource* FrameResource::GetResource(void* targetComponent, IPipelineResource* (*func)(void const*), void const* funcPtr)
{
	std::lock_guard<std::mutex> lck(mtx);
	auto ite = perFrameResource.Find(targetComponent);
	if (!ite)
	{
		IPipelineResource* newComp = func(funcPtr);
		perFrameResource.Insert(targetComponent, newComp);
		return newComp;
	}
	return ite.Value();
}
IPipelineResource* FrameResource::GetPerCameraResource(void* targetComponent, Camera* cam, IPipelineResource* (*func)(void const*), void const* funcPtr)
{
	std::lock_guard<std::mutex> lck(mtx);
	FrameResCamera* ptr = perCameraDatas[cam->GetInstanceID()];
	auto ite = ptr->perCameraResource.Find(targetComponent);
	if (!ite)
	{
		IPipelineResource* newComp = func(funcPtr);
		ptr->perCameraResource.Insert(targetComponent, newComp);
		return newComp;
	}
	return ite.Value();
}