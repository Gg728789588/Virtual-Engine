#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/MathHelper.h"
#include "../RenderComponent/UploadBuffer.h"
#include "../RenderComponent/CBufferPool.h"
#include <functional>
#include "../PipelineComponent/ThreadCommand.h"
#include "../Common/Pool.h"
#include "../JobSystem/JobSystem.h"
#include "../PipelineComponent/IPipelineResource.h"
#include "../Common/MetaLib.h"
#include <mutex>
#include "../PipelineComponent/CommandBuffer.h"
#include "../Common/TypeWiper.h"
#include "../Common/Runnable.h"
class Camera;
class PipelineComponent;


// Stores the resources needed for the CPU to build the command lists
// for a frame.  
struct FrameResource
{
private:
	struct FrameResCamera
	{
		HashMap<void*, IPipelineResource*> perCameraResource;
		ArrayList<ThreadCommand*> graphicsThreadCommands;
		ArrayList<ThreadCommand*> computeThreadCommands;
		ArrayList<ThreadCommand*> copyThreadCommands;
		FrameResCamera() :
			perCameraResource(53)
		{
			graphicsThreadCommands.reserve(20);
			copyThreadCommands.reserve(20);
			computeThreadCommands.reserve(20);
		}
		~FrameResCamera()
		{
			perCameraResource.IterateAll([](uint i, void* const key, IPipelineResource* value)->void
				{
					delete value;
				});
		}
	};
	static Pool<ThreadCommand> threadCommandMemoryPool;
	static   Pool<FrameResCamera> perCameraDataMemPool;
	static   vengine::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> needClearResourcesAfterFlush;
	static vengine::vector<Runnable<void()>> needDisposeResourcesAfterFlush;
	vengine::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> needClearResources;
	vengine::vector<Runnable<void()>> needDisposeResources;
	HashMap<uint32_t, FrameResCamera*> perCameraDatas;
	HashMap<void*, IPipelineResource*> perFrameResource;
	ArrayList<uint32_t> unloadCommands;
	std::mutex mtx;
	void DisposeResource(void* targetComponent);
	void DisposePerCameraResource(void* targetComponent, Camera* cam);
	IPipelineResource* GetResource(void* targetComponent, IPipelineResource* (*func)(void const*), void const* ptr);
	IPipelineResource* GetPerCameraResource(void* targetComponent, Camera* cam, IPipelineResource* (*func)(void const*), void const* ptr);
public:
	ThreadCommand* commmonThreadCommand;
	ThreadCommand* copyThreadCommand;
	static CBufferPool cameraCBufferPool;
	static vengine::vector<std::unique_ptr<FrameResource>> mFrameResources;
	static FrameResource* mCurrFrameResource;
	CommandBuffer commandBuffer;
	FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource();
	void UpdateBeforeFrame(const std::initializer_list<ID3D12Fence*>& fences);
	void ReleaseResourceAfterFlush(const Microsoft::WRL::ComPtr<ID3D12Resource>& resources);
	void DisposeTargetAfterFlush(Runnable<void()> const& disposer);
	void UpdateAfterFrame(UINT64& currentFence, const std::initializer_list<std::pair< ID3D12CommandQueue*, ID3D12Fence*>>& commandQueues);
	ThreadCommand* GetNewThreadCommand(Camera* cam, ID3D12Device* device, D3D12_COMMAND_LIST_TYPE cmdListType);
	void ReleaseThreadCommand(Camera* cam, ThreadCommand* targetCmd, D3D12_COMMAND_LIST_TYPE cmdListType);
	void OnLoadCamera(Camera* targetCamera, ID3D12Device* device);
	void OnUnloadCamera(Camera* targetCamera);
	// We cannot reset the allocator until the GPU is done processing the commands.
	// So each frame needs their own allocator.
	//Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;
	// We cannot update a cbuffer until the GPU is done processing the commands
	// that reference it.  So each frame needs their own cbuffers.
   // std::unique_ptr<UploadBuffer<FrameConstants>> FrameCB = nullptr;
	HashMap<uint, ConstBufferElement> cameraCBs;
	HashMap<UINT, ConstBufferElement> objectCBs;
	// Fence value to mark commands up to this fence point.  This lets us
	// check if these frame resources are still in use by the GPU.
	UINT64 Fence = 0;
	//Rendering Events
	template <typename Func>
	inline IPipelineResource* GetPerCameraResource(void* targetComponent, Camera* cam, const Func& func)
	{
		return GetPerCameraResource(targetComponent, cam, GetFunctorPointer_Const<Func, IPipelineResource*>(), &func);
	}
	template <typename Func>
	inline IPipelineResource* GetResource(void* targetComponent, const Func& func)
	{
		return GetResource(targetComponent, GetFunctorPointer_Const<Func, IPipelineResource*>(), &func);
	}
	static void DisposeResources(void* targetComponent);
	static void DisposePerCameraResources(void* targetComponent, Camera* cam);
};