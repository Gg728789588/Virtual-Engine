#include "ThreadCommand.h"
#include "../RenderComponent/RenderTexture.h"
#include "../RenderComponent/StructuredBuffer.h"
#include "../Singleton/Graphics.h"
#include "../Common/d3dApp.h"
void ThreadCommand::ResetCommand()
{
	ThrowIfFailed(cmdAllocator->Reset());
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));
}
void ThreadCommand::CloseCommand()
{
	rtStateMap.IterateAll([&](uint i, RenderTexture* const key, ResourceReadWriteState& value) -> void
		{
			if (value)
			{
				D3D12_RESOURCE_STATES writeState, readState;
				writeState = key->GetWriteState();
				readState = key->GetReadState();
				buffer.AddCommand(readState, writeState, key->GetResource());
			}
		});
	sbufferStateMap.IterateAll([&](uint i, StructuredBuffer* const key, ResourceReadWriteState& value) -> void
		{
			if (value)
			{
				buffer.AddCommand(D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, key->GetResource());
			}
		});
	buffer.Clear(GetCmdList());
	rtStateMap.Clear();
	sbufferStateMap.Clear();

	cmdList->Close();
}
ThreadCommand::ThreadCommand(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type) : 
	rtStateMap(23), sbufferStateMap(23)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		type,
		IID_PPV_ARGS(&cmdAllocator)));
	ThrowIfFailed(device->CreateCommandList(
		0,
		type,
		cmdAllocator.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject
		IID_PPV_ARGS(&cmdList)));
	cmdList->Close();
}

bool ThreadCommand::UpdateState(RenderTexture* rt, ResourceReadWriteState state)
{
	auto ite = rtStateMap.Find(rt);
	if (state)
	{
		if (!ite)
		{
			rtStateMap.Insert(rt, state);
		}
		else if (ite.Value() != state)
		{
			ite.Value() = state;
		}
		else
			return false;
	}
	else
	{
		if (ite)
		{
			rtStateMap.Remove(ite);
		}
		else
			return false;
	}
	return true;
}
bool ThreadCommand::UpdateState(StructuredBuffer* rt, ResourceReadWriteState state)
{
	auto ite = sbufferStateMap.Find(rt);
	if (state)
	{
		if (!ite)
		{
			sbufferStateMap.Insert(rt, state);
		}
		else if (ite.Value() != state)
		{
			ite.Value() = state;
		}
		else
			return false;
	}
	else
	{
		if (ite)
		{
			sbufferStateMap.Remove(ite);
		}
		else
			return false;
	}
	return true;
}

void ThreadCommand::SetResourceReadWriteState(RenderTexture* rt, ResourceReadWriteState state)
{
	if (!UpdateState(rt, state)) return;
	D3D12_RESOURCE_STATES writeState, readState;
	writeState = rt->GetWriteState();
	readState = rt->GetReadState();
	if (state)
	{
		buffer.AddCommand(writeState, readState, rt->GetResource());
	}
	else
	{
		buffer.AddCommand( readState, writeState, rt->GetResource());
	}
}

void ThreadCommand::SetResourceReadWriteState(StructuredBuffer* rt, ResourceReadWriteState state)
{
	if (!UpdateState(rt, state)) return;
	if (state)
	{
		buffer.AddCommand(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ, rt->GetResource());
	}
	else
	{
		buffer.AddCommand(D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, rt->GetResource());
	}
}