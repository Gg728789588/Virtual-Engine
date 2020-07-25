#include "RenderCommand.h"
#include "../Singleton/FrameResource.h"

std::mutex RenderCommand::mtx;
RingQueue<RenderCommandExecutable> RenderCommand::queue(100);

void RenderCommand::AddCommand(
	RenderCommand* ptr,
		PoolBase* commandPool,
		std::mutex* poolMutex)
{
	std::lock_guard<std::mutex> lck(mtx);
	queue.EmplacePush<RenderCommandExecutable>({ptr, commandPool, poolMutex});
}

bool RenderCommand::ExecuteCommand(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* directCommandList,
	ID3D12GraphicsCommandList* copyCommandList,
	FrameResource* resource,
	TransitionBarrierBuffer* barrierBuffer)
{
	RenderCommandExecutable ptr;
	bool v = false;
	{
		std::lock_guard<std::mutex> lck(mtx);
		v = queue.TryPop(&ptr);
	}
	if (!v) return false;
	ptr.ptr->Execute(device, directCommandList, copyCommandList, resource, barrierBuffer);
	{
		ptr.commandPool->Delete_Lock(*ptr.poolMutex, ptr.ptr);
	}
	return true;
}