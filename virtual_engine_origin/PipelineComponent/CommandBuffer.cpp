#include "CommandBuffer.h"


void CommandBuffer::Clear()
{
	executeCommands.clear();
	graphicsCmdLists.clear();
}

void CommandBuffer::Wait(ID3D12CommandQueue* queue, ID3D12Fence* computeFence, UINT64 currentFrame)
{
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_Wait;
	cmd.targetQueue = queue;
	cmd.waitFence.fence = computeFence;
	cmd.waitFence.frameIndex = currentFrame;
	executeCommands.push_back(cmd);
}

void CommandBuffer::Signal(ID3D12CommandQueue* queue, ID3D12Fence* computeFence, UINT64 currentFrame)
{
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_Signal;
	cmd.targetQueue = queue;
	cmd.signalFence.fence = computeFence;
	cmd.signalFence.frameIndex = currentFrame;
	executeCommands.push_back(cmd);
}

void CommandBuffer::Execute(ID3D12CommandQueue* queue, ID3D12GraphicsCommandList* cmdList)
{
	InnerCommand cmd;
	cmd.type = InnerCommand::CommandType_Execute;
	cmd.targetQueue = queue;
	cmd.executeCmdList = cmdList;
	executeCommands.push_back(cmd);
}

void CommandBuffer::Submit()
{
	ID3D12CommandQueue* currentQueue = nullptr;
	auto clearQueue = [&]()->void
	{
		if (currentQueue)
		{
			currentQueue->ExecuteCommandLists(graphicsCmdLists.size(), (ID3D12CommandList* const*)graphicsCmdLists.data());
			graphicsCmdLists.clear();
			currentQueue = nullptr;
		}
	};
	auto pushQueue = [&](ID3D12CommandQueue* targetQueue, ID3D12GraphicsCommandList* cmdList)->void
	{
		if (currentQueue != targetQueue)
		{
			clearQueue();
		}
		currentQueue = targetQueue;
		graphicsCmdLists.push_back(cmdList);
	};
	for (uint i = 0; i < executeCommands.size(); ++i)
	{
		auto& ite = executeCommands[i];
		switch (ite.type)
		{
		case InnerCommand::CommandType_Execute:
		{
			pushQueue(ite.targetQueue, ite.executeCmdList);
		}
		break;
		case InnerCommand::CommandType_Signal:
		{
			clearQueue();
			ite.targetQueue->Signal(ite.signalFence.fence, ite.signalFence.frameIndex);
		}
		break;
		case InnerCommand::CommandType_Wait:
		{
			clearQueue();
			ite.targetQueue->Wait(ite.waitFence.fence, ite.waitFence.frameIndex);
		}
		break;
		}
	}
	clearQueue();
}

CommandBuffer::CommandBuffer()
{
	graphicsCmdLists.reserve(20);
	executeCommands.reserve(20);
}