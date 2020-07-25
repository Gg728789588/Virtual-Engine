#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/ArrayList.h"
class CommandBuffer
{
private:
	ArrayList<ID3D12GraphicsCommandList*> graphicsCmdLists;
	struct Fence
	{
		ID3D12Fence* fence;
		UINT64 frameIndex;
	};
	struct InnerCommand
	{
		enum CommandType
		{
			CommandType_Execute,
			CommandType_Signal,
			CommandType_Wait,
		};
		CommandType type;
		ID3D12CommandQueue* targetQueue;
		union
		{
			ID3D12GraphicsCommandList* executeCmdList;
			Fence waitFence;
			Fence signalFence;
		};
		InnerCommand() {}
		InnerCommand(const InnerCommand& cmd)
		{
			memcpy(this, &cmd, sizeof(InnerCommand));
		}
		void operator=(const InnerCommand& cmd)
		{
			memcpy(this, &cmd, sizeof(InnerCommand));
		}
	};
	ArrayList<InnerCommand> executeCommands;
public:
	void Wait(ID3D12CommandQueue* queue, ID3D12Fence* computeFence, UINT64 currentFrame);
	void Signal(ID3D12CommandQueue* queue, ID3D12Fence* computeFence, UINT64 currentFrame);
	void Execute(ID3D12CommandQueue* queue, ID3D12GraphicsCommandList* cmdList);
	void Submit();
	void Clear();
	CommandBuffer();
};