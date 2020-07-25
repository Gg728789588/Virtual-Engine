#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/Pool.h"
#include <mutex>
#include "../Common/RingQueue.h"
class TransitionBarrierBuffer;
class FrameResource;
class RenderCommand;
struct RenderCommandExecutable
{
	RenderCommand* ptr = nullptr;
	PoolBase* commandPool = nullptr;
	std::mutex* poolMutex = nullptr;

};
class RenderCommand
{
private:
	static std::mutex mtx;
	static RingQueue<RenderCommandExecutable> queue;
public:
	virtual ~RenderCommand() {}
	/*	
		Execute a Render Command with async queue
		Direct Queue Waiting for compute & copy queue
		TransitionBarrierBuffer belongs to direct queue
	*/

	virtual void Execute(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* directCommandList,
		ID3D12GraphicsCommandList* copyCommandList,
		FrameResource* resource,
		TransitionBarrierBuffer*) = 0;
	static void AddCommand(
		RenderCommand* ptr,
		PoolBase* commandPool,
		std::mutex* poolMutex);
	static bool ExecuteCommand(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* directCommandList,
		ID3D12GraphicsCommandList* copyCommandList,
		FrameResource* resource,
		TransitionBarrierBuffer* barrierBuffer);
};