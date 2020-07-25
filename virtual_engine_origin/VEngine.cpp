//***************************************************************************************
// VEngine.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************
#include "RenderComponent/Shader.h"
#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "RenderComponent/UploadBuffer.h"
#include "Singleton/FrameResource.h"
#include "Singleton/ShaderID.h"
#include "Common/Input.h"
#include "RenderComponent/Texture.h"
#include "Singleton/MeshLayout.h"
#include "Singleton/PSOContainer.h"
#include "Common/Camera.h"
#include "RenderComponent/Mesh.h"
#include "RenderComponent/MeshRenderer.h"
#include "Singleton/ShaderCompiler.h"
#include "RenderComponent/Skybox.h"
#include "RenderComponent/ComputeShader.h"
#include "RenderComponent/RenderTexture.h"
#include "PipelineComponent/RenderPipeline.h"
#include "Singleton/Graphics.h"
#include "Singleton/MathLib.h"
#include "JobSystem/JobInclude.h"
#include "ResourceManagement/AssetDatabase.h"
using Microsoft::WRL::ComPtr;
using namespace Math;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

#include "LogicComponent/World.h"
#include "IRendererBase.h"

ID3D12Resource* CurrentBackBuffer_VEngine(VEngine const* ve, const D3DAppDataPack& dataPack);
D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView_VEngine(VEngine const* ve, const D3DAppDataPack& dataPack);
class VEngine final : public IRendererBase
{
public:
	ArrayList<JobBucket*> buckets[2];
	bool bucketsFlag = false;
#pragma region CORE_DirectX
	Microsoft::WRL::ComPtr < IDXGISwapChain> mSwapChain;//To Engine
	Microsoft::WRL::ComPtr <ID3D12CommandQueue> mCommandQueue;//To Engine
	Microsoft::WRL::ComPtr < ID3D12Resource> mSwapChainBuffer[3];//To Engine
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> mRtvHeap;//To Engine
	//D3D12_VIEWPORT mScreenViewport;
	//D3D12_RECT mScissorRect;
#pragma endregion

	ComPtr<ID3D12CommandQueue> mComputeCommandQueue;
	ComPtr<ID3D12Fence> directCommandQueueWaitFence;
	ComPtr<ID3D12CommandQueue> mAsyncCopyCommandQueue;
	ComPtr<ID3D12CommandQueue> mCopyCommandQueue;
	ComPtr<ID3D12Fence> copyCommandQueueWaitFence;
	ComPtr<ID3D12Fence> copyToDirectFence;
	INLINE VEngine(const VEngine& rhs) = delete;
	INLINE VEngine() {

	}
	void CreateCommandObjects(D3DAppDataPack& dataPack)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		ThrowIfFailed(dataPack.md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
	}
	void CreateFences(D3DAppDataPack& pack)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

		ThrowIfFailed(pack.md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mComputeCommandQueue)));
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		ThrowIfFailed(pack.md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mAsyncCopyCommandQueue)));
		ThrowIfFailed(pack.md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCopyCommandQueue)));

		ThrowIfFailed(pack.md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyToDirectFence)));
		ThrowIfFailed(pack.md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&directCommandQueueWaitFence)));
		ThrowIfFailed(pack.md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyCommandQueueWaitFence)));
	}
	void CreateSwapChain(D3DAppDataPack& dataPack)
	{
		// Release the previous swapchain we will be recreating.
		if (mSwapChain)
		{
			mSwapChain->Release();
			mSwapChain = nullptr;
		}

		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferDesc.Width = dataPack.settings.mClientWidth;
		sd.BufferDesc.Height = dataPack.settings.mClientHeight;
		sd.BufferDesc.RefreshRate.Numerator = 144;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = dataPack.settings.backBufferFormat;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = gNumFrameResources;
		sd.OutputWindow = dataPack.mhMainWnd;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		HRESULT r = (dataPack.mdxgiFactory->CreateSwapChain(
			mCommandQueue.Get(),
			&sd,
			&mSwapChain));
		size_t t = 0;
	}
	void CreateRtvAndDsvDescriptorHeaps(D3DAppDataPack& dataPack)
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
		rtvHeapDesc.NumDescriptors = gNumFrameResources;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		ThrowIfFailed(dataPack.md3dDevice->CreateDescriptorHeap(
			&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));
	}
	INLINE void flushCommandQueue(D3DAppDataPack& dataPack)
	{
		// Advance the fence value to mark commands up to this fence point.
		dataPack.settings.mCurrentFence++;

		// Add an instruction to the command queue to set a new fence point.  Because we 
		// are on the GPU timeline, the new fence point won't be set until the GPU finishes
		// processing all the commands prior to this Signal().
		ThrowIfFailed(mCommandQueue->Signal(directCommandQueueWaitFence.Get(), dataPack.settings.mCurrentFence));
		if (directCommandQueueWaitFence->GetCompletedValue() < dataPack.settings.mCurrentFence)
		{
			LPCWSTR falseValue = (LPCWSTR)false;
			HANDLE eventHandle = CreateEventEx(nullptr, falseValue, false, EVENT_ALL_ACCESS);

			// Fire event when GPU hits current fence.  
			ThrowIfFailed(directCommandQueueWaitFence->SetEventOnCompletion(dataPack.settings.mCurrentFence, eventHandle));

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
		// Wait until the GPU has completed commands up to this fence point.
	};

	~VEngine()
	{

	}
	INLINE VEngine& operator=(const VEngine& rhs) = delete;
	INLINE void DisposeRenderer()
	{
		FrameResource::mFrameResources.clear();

		RenderPipeline::DestroyInstance();
		ShaderCompiler::Dispose();
	}
	virtual bool __cdecl Initialize(D3DAppDataPack& dataPack)
	{
		directThreadCommand.New(dataPack.md3dDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		CreateFences(dataPack);
		CreateCommandObjects(dataPack);
		CreateSwapChain(dataPack);
		CreateRtvAndDsvDescriptorHeaps(dataPack);

		//pack.mSwapChain->SetFullscreenState(true, nullptr);
		Graphics::Initialize(dataPack.md3dDevice.Get(), directThreadCommand->GetCmdList());
		ShaderID::Init();
		buckets[0].reserve(20);
		buckets[1].reserve(20);
		int cpuCoreCount = std::thread::hardware_concurrency() - 2;	//One for main thread & one for loading
		pipelineJobSys.New(Max<int>(1, cpuCoreCount));
		AssetDatabase::CreateInstance(dataPack.md3dDevice.Get(), dataPack.adapter);

		directThreadCommand->ResetCommand();
		// Reset the command list to prep for initialization commands.
		ShaderCompiler::Init(dataPack.md3dDevice.Get(), pipelineJobSys.GetPtr());
		BuildFrameResources(dataPack);

		//BuildPSOs();
		// Execute the initialization commands.
		// Wait until initialization is complete.

		World::CreateInstance(directThreadCommand->GetCmdList(), dataPack.md3dDevice.Get());

		rp = RenderPipeline::GetInstance(dataPack.md3dDevice.Get(),
			directThreadCommand->GetCmdList());
		directThreadCommand->CloseCommand();
		ID3D12CommandList* lst = directThreadCommand->GetCmdList();
		mCommandQueue->ExecuteCommandLists(1, &lst);
		flushCommandQueue(dataPack);
		directThreadCommand.Delete();
		return true;
	}
	virtual void __cdecl OnResize(D3DAppDataPack& dataPack)
	{
		if (dataPack.settings.mCurrentFence > 1)
		{
			auto& vec = FrameResource::mFrameResources;
			for (auto ite = vec.begin(); ite != vec.end(); ++ite)
			{
				(*ite)->commandBuffer.Submit();
				(*ite)->commandBuffer.Clear();
			}
			flushCommandQueue(dataPack);
		}
		assert(mSwapChain);
		// Release the previous resources we will be recreating.
		for (int i = 0; i < gNumFrameResources; ++i)
		{
			mSwapChainBuffer[i] = nullptr;

		}

		// Resize the swap chain.
		ThrowIfFailed(mSwapChain->ResizeBuffers(
			gNumFrameResources,
			dataPack.settings.mClientWidth, dataPack.settings.mClientHeight,
			dataPack.settings.backBufferFormat,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		dataPack.settings.mCurrBackBuffer = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < gNumFrameResources; i++)
		{
			ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
			dataPack.md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
			rtvHeapHandle.Offset(1, dataPack.settings.mRtvDescriptorSize);
		}
		// Update the viewport transform to cover the client area.
		/*mScreenViewport.TopLeftX = 0;
		mScreenViewport.TopLeftY = 0;
		mScreenViewport.Width = static_cast<float>(dataPack.settings.mClientWidth);
		mScreenViewport.Height = static_cast<float>(dataPack.settings.mClientHeight);
		mScreenViewport.MinDepth = 0.0f;
		mScreenViewport.MaxDepth = 1.0f;

		mScissorRect = { 0, 0, dataPack.settings.mClientWidth, dataPack.settings.mClientHeight };*/
	}
	virtual bool __cdecl Draw(D3DAppDataPack& pack, GameTimer& timer)
	{
		if (pack.settings.mClientHeight < 1 || pack.settings.mClientWidth < 1) return true;
		mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
		lastResource = FrameResource::mCurrFrameResource;
		FrameResource::mCurrFrameResource = FrameResource::mFrameResources[mCurrFrameResourceIndex].get();
		auto currentFrameResource = FrameResource::mCurrFrameResource;
		ArrayList <JobBucket*>& bucketArray = buckets[bucketsFlag];
		bucketsFlag = !bucketsFlag;
		JobBucket* mainLogicBucket = pipelineJobSys->GetJobBucket();
		bucketArray.push_back(mainLogicBucket);

		//Rendering
		pack.settings.windowInfo = const_cast<wchar_t*>(World::GetInstance()->windowInfo.empty() ? nullptr : World::GetInstance()->windowInfo.c_str());
		pack.settings.mCurrBackBuffer = (pack.settings.mCurrBackBuffer + 1) % gNumFrameResources;
		RenderPipelineData data;
		data.device = pack.md3dDevice.Get();
		data.directQueue = mCommandQueue.Get();
		data.computeQueue = mComputeCommandQueue.Get();
		data.asyncCopyQueue = mAsyncCopyCommandQueue.Get();
		data.copyQueue = mCopyCommandQueue.Get();
		data.copyToDirectFence = copyToDirectFence.Get();
		data.backBufferHandle = CurrentBackBufferView_VEngine(this, pack);
		data.copyFence = copyCommandQueueWaitFence.Get();
		data.backBufferResource = CurrentBackBuffer_VEngine(this, pack);
		data.lastResource = lastResource;
		data.resource = currentFrameResource;
		data.allCameras = &World::GetInstance()->GetCameras();
		data.fence = directCommandQueueWaitFence.Get();
		data.fenceIndex = &pack.settings.mCurrentFence;
		data.ringFrameIndex = mCurrFrameResourceIndex;
		data.world = World::GetInstance();
		data.world->windowWidth = pack.settings.mClientWidth;
		data.world->windowHeight = pack.settings.mClientHeight;
		data.deltaTime = timer.DeltaTime();
		data.time = timer.TotalTime();
		data.isMovingTheWorld = false;
		data.worldMovingDir = { 0,0,0 };
		/*if (Input::IsKeyDown(KeyCode::M))
		{
			data.isMovingTheWorld = true;
			data.worldMovingDir = { 10, 10, 10 };
		}*/
		Vector3 worldMoveDir;
		
		uint64 frameIndex = *data.fenceIndex;
		
	
		mainLogicBucket->GetTask({}, [&, currentFrameResource, worldMoveDir, frameIndex]()->void
			{
				World::GetInstance()->Update(currentFrameResource, pack.md3dDevice.Get(), timer, frameIndex, int2(pack.settings.mClientWidth, pack.settings.mClientHeight), worldMoveDir);

			});
		rp->PrepareRendering(data, pipelineJobSys.GetPtr(), bucketArray);

		HRESULT crashResult = pack.md3dDevice->GetDeviceRemovedReason();
		if (crashResult != S_OK)
		{
			return false;
		}
		Input::UpdateFrame(int2(-1, -1));//Update Input Buffer
	//	SetCursorPos(mClientWidth / 2, mClientHeight / 2);
		data.resource->UpdateBeforeFrame({
			data.fence,
			data.copyFence
			});//Flush CommandQueue
		pipelineJobSys->Wait();//Last Frame's Logic Stop Here
		pipelineJobSys->ExecuteBucket(bucketArray.data(), bucketArray.size());					//Execute Tasks
		HRESULT presentResult = rp->ExecuteRendering(data, mSwapChain.Get());
#if defined(DEBUG) | defined(_DEBUG)
		ThrowHResult(presentResult, PresentFunction);
#endif
		if (presentResult != S_OK)
		{
			return false;
		}

		ArrayList <JobBucket*>& lastBucketArray = buckets[bucketsFlag];
		for (auto ite = lastBucketArray.begin(); ite != lastBucketArray.end(); ++ite)
		{
			pipelineJobSys->ReleaseJobBucket(*ite);
		}

		lastBucketArray.clear();
		return true;
	}
	virtual void __cdecl Dispose(D3DAppDataPack& pack)
	{
		pipelineJobSys->Wait();
		if (pack.md3dDevice != nullptr)
			flushCommandQueue(pack);
		World::GetInstance()->DestroyAllCameras();
		DisposeRenderer();
		pipelineJobSys.Delete();
		World::DestroyInstance();
		mComputeCommandQueue = nullptr;
		mAsyncCopyCommandQueue = nullptr;
		mCopyCommandQueue = nullptr;
		mSwapChain->SetFullscreenState(false, nullptr);
		mSwapChain = nullptr;
		mCommandQueue = nullptr;
		for (uint i = 0; i < gNumFrameResources; ++i)
			mSwapChainBuffer[i] = nullptr;
		mRtvHeap = nullptr;
	}
	//void AnimateMaterials(const GameTimer& gt);
	//void UpdateObjectCBs(const GameTimer& gt);

	StackObject<JobSystem> pipelineJobSys;
	INLINE void BuildFrameResources(D3DAppDataPack& pack)
	{
		FrameResource::mFrameResources.resize(gNumFrameResources);
		for (int i = 0; i < gNumFrameResources; ++i)
		{
			FrameResource::mFrameResources[i] = std::unique_ptr<FrameResource>(new FrameResource(pack.md3dDevice.Get(), 1, 1));
		}
	}
	int mCurrFrameResourceIndex = 0;
	RenderPipeline* rp;

	float mTheta = 1.3f * XM_PI;
	float mPhi = 0.4f * XM_PI;
	float mRadius = 2.5f;
	POINT mLastMousePos;
	FrameResource* lastResource = nullptr;
	StackObject<ThreadCommand> directThreadCommand;
};
IRendererBase* __cdecl GetRenderer()
{
	return new VEngine();
}


ID3D12Resource* CurrentBackBuffer_VEngine(VEngine const* ve, const D3DAppDataPack& dataPack)
{
	return ve->mSwapChainBuffer[dataPack.settings.mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView_VEngine(VEngine const* ve, const D3DAppDataPack& dataPack)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		ve->mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		dataPack.settings.mCurrBackBuffer,
		dataPack.settings.mRtvDescriptorSize);
}
