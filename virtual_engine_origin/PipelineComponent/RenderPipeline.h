#pragma once
#include "TempRTAllocator.h"
#include "../Common/d3dUtil.h"
#include "../Common/MetaLib.h"
#include "../Common/HashMap.h"
#include "../Common/ArrayList.h"
#include "../Utility/TaskThread.h"
class FrameResource;
class Camera;
class World;
class JobSystem;
class PipelineComponent;
class JobBucket;
class JobHandle;
struct RenderPipelineData
{
	ID3D12Device* device;
	ID3D12CommandQueue* directQueue;
	ID3D12CommandQueue* computeQueue;
	ID3D12CommandQueue* asyncCopyQueue;
	ID3D12CommandQueue* copyQueue;
	ID3D12Fence* copyToDirectFence;
	ID3D12Fence* copyFence;
	ID3D12Resource* backBufferResource;
	D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle;
	FrameResource* lastResource;
	FrameResource* resource;
	vengine::vector<ObjectPtr<Camera>>* allCameras;
	ID3D12Fence* fence;
	UINT64* fenceIndex;
	World* world;
	float time;
	float deltaTime;
	uint ringFrameIndex;
	float3 worldMovingDir;
	bool isMovingTheWorld;
};
class RenderPipeline final
{
private:
	struct AfterRenderEvent
	{
		Camera* cam;
		void operator()() const noexcept;
	};

	static RenderPipeline* current;
	struct RenderTextureMark
	{
		UINT id;
		UINT rtIndex;
		ResourceDescriptor desc;
		UINT startComponent;
		UINT endComponent;
	};
	ArrayList<Camera*> manualRenderList;
	std::mutex renderListMtx;
	ArrayList<PipelineComponent*> components;
	ArrayList<JobHandle> fenceCounter;
	TempRTAllocator tempRTAllocator;
	HashMap<Type, PipelineComponent*> componentsLink;
	vengine::vector<ArrayList<PipelineComponent*>> renderPathComponents;
	HashMap<UINT, RenderTextureMark> renderTextureMarks;
	UINT initCount = 0;
	void mInit(PipelineComponent* ptr, const std::type_info& tp);

	template <typename ... Args>
	void Init()
	{
		char c[] = { (mInit(new Args(), typeid(Args)), 0)... };
	}
	RenderPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
public:
	static RenderPipeline* GetInstance(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	static void DestroyInstance();
	~RenderPipeline();
	void PrepareRendering(RenderPipelineData& data, JobSystem* jobSys, ArrayList <JobBucket*>& bucketArray) noexcept;
	HRESULT ExecuteRendering(RenderPipelineData& data, IDXGISwapChain* swapChain) noexcept;
	static void AddCameraRenderCommand(Camera* cam);

	template <typename T>
	static T* GetComponent()
	{
		auto ite = current->componentsLink.Find(typeid(T));
		if (ite)
			return (T*)ite.Value();
		else return nullptr;
	}
};