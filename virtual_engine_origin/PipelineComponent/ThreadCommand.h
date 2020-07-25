#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/HashMap.h"
#include <mutex>
#include <atomic>
#include "../RenderComponent/TransitionBarrierBuffer.h"
struct StateTransformBuffer
{
	ID3D12Resource* targetResource;
	D3D12_RESOURCE_STATES beforeState;
	D3D12_RESOURCE_STATES afterState;
};
class PipelineComponent;
class StructuredBuffer;
class RenderTexture;

class ThreadCommand final
{
	friend class PipelineComponent;
private:
	TransitionBarrierBuffer buffer;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
	HashMap<RenderTexture*, ResourceReadWriteState> rtStateMap;
	HashMap<StructuredBuffer*, ResourceReadWriteState> sbufferStateMap;
	bool UpdateState(RenderTexture* rt, ResourceReadWriteState state);
	bool UpdateState(StructuredBuffer* rt, ResourceReadWriteState state);
public:
	TransitionBarrierBuffer* GetBarrierBuffer()
	{
		return &buffer;
	}
	inline ID3D12CommandAllocator* GetAllocator() const { return cmdAllocator.Get(); }
	inline ID3D12GraphicsCommandList* GetCmdList() const { return cmdList.Get(); }
	ThreadCommand(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
	void SetResourceReadWriteState(RenderTexture* rt, ResourceReadWriteState state);
	void SetResourceReadWriteState(StructuredBuffer* rt, ResourceReadWriteState state);

	void ResetCommand();
	void CloseCommand();
};

class ThreadCommandFollower
{
private:
	ThreadCommand* cmd;
public:
	ThreadCommandFollower(ThreadCommand* cmd) : cmd(cmd) {
		cmd->ResetCommand();
	}
	ThreadCommandFollower(const ThreadCommandFollower&) = delete;
	ThreadCommandFollower(ThreadCommandFollower&&) = delete;
	void operator=(const ThreadCommandFollower&) = delete;
	void operator=(ThreadCommandFollower&&) = delete;
	~ThreadCommandFollower()
	{
		cmd->CloseCommand();
	}
};