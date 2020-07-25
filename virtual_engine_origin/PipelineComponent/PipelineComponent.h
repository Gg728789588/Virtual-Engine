#pragma once
#include "../Singleton/FrameResource.h"
#include "../JobSystem/JobInclude.h"
#include "RenderPipeline.h"
class ThreadCommand;
class FrameResource;
class TempRTAllocator;
class RenderTexture;
class World;

struct TemporalResourceCommand
{
	enum CommandType
	{
		CommandType_Create_RenderTexture,
		CommandType_Require_RenderTexture,
		CommandType_Create_StructuredBuffer,
		CommandType_Require_StructuredBuffer
	};
	CommandType type;
	UINT uID;
	ResourceDescriptor descriptor;
	TemporalResourceCommand()
	{
		memset(this, 0, sizeof(TemporalResourceCommand));
	}
	TemporalResourceCommand(const TemporalResourceCommand& cmd)
	{
		memcpy(this, &cmd, sizeof(TemporalResourceCommand));
	}
	void operator=(const TemporalResourceCommand& cmd)
	{
		memcpy(this, &cmd, sizeof(TemporalResourceCommand));
	}

	//bool operator==(const TemporalResourceCommand& other) const;
};
class PerCameraRenderingEvent;
class CommandBuffer;
enum CommandListType
{
	CommandListType_None = 0,
	CommandListType_Graphics = 1,
	CommandListType_Compute = 2,
	CommandListType_Copy = 3
};

struct RequiredRT
{
	UINT descIndex;
	UINT uID;
};
class PipelineComponent
{
	friend class RenderPipeline;
	friend class PerCameraRenderingEvent;
private:
	static std::mutex mtx;
	static JobBucket* bucket;
	ThreadCommand* threadCommand;//thread command cache
	struct LoadTempRTCommand
	{
		UINT uID;
		UINT index;
		ResourceDescriptor descriptor;
	};
	ArrayList<LoadTempRTCommand> loadRTCommands;
	ArrayList<UINT> unLoadRTCommands;
	ArrayList<PipelineComponent*> cpuDepending;
	ArrayList<PipelineComponent*> gpuDepending;
	ArrayList<JobHandle> dependingNodes;
	ArrayList<RequiredRT> requiredRTs;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	UINT dependingComponentCount = 0;
	void ExecuteTempRTCommand(ID3D12Device* device, TempRTAllocator* allocator);
protected:

	ArrayList<JobHandle> jobHandles;
	ArrayList<MObject*> allTempResource;
	void CreateFence(ID3D12Device* device);
	template <typename... Args>
	void SetCPUDepending()
	{
		cpuDepending.clear();
		cpuDepending.reserve(sizeof...(Args));
		char c[] = { (cpuDepending.push_back(RenderPipeline::GetComponent<Args>()), 0)... };

	}
	template <typename... Args>
	void SetGPUDepending()
	{
		gpuDepending.clear();
		gpuDepending.reserve(sizeof...(Args));
		char c[] = { (gpuDepending.push_back(RenderPipeline::GetComponent<Args>()), 0)... };
	}
public:

	struct EventData
	{
		ID3D12Device* device;
		ID3D12Resource* backBuffer;
		Camera* camera;
		FrameResource* resource;
		World* world;
		uint64 frameCount;
		D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
		UINT width, height;
		float deltaTime;
		float time;
		uint ringFrameIndex;
		float3 worldMovingDir;
		bool isMovingTheWorld;
		bool isBackBufferForPresent;
	};
	template <typename Func>
	JobHandle ScheduleJob(const Func& func)
	{
		JobHandle handle = bucket->GetTask(dependingNodes.data(), dependingNodes.size(), func);
		jobHandles.push_back(handle);
		return handle;
	}

	virtual CommandListType GetCommandListType() = 0;
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) = 0;
	virtual void Dispose() = 0;
	virtual ArrayList<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt) = 0;
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList) = 0;
	void ClearHandles();
	void MarkDepending(JobHandle extraNode);
	~PipelineComponent() {}
	PipelineComponent();
};