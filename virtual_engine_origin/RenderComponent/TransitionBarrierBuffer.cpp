#include "TransitionBarrierBuffer.h"
TransitionBarrierBuffer::TransitionBarrierBuffer() : 
	barrierRecorder(23),
	uavBarriersDict(23)
{
	commands.reserve(20);
	uavBarriers.reserve(20);
}

void TransitionBarrierBuffer::RegistInitState(D3D12_RESOURCE_STATES initState, ID3D12Resource* resource)
{
	Command& cmd = barrierRecorder[resource];
	if (!&cmd)
	{
		barrierRecorder.Insert(resource, Command(initState, -1));
	}
	else
	{
#if defined(DEBUG) || defined(_DEBUG)
		if (cmd.targetState != initState)
			throw "Re-Regist!";
#endif
	}
}

void TransitionBarrierBuffer::UpdateState(D3D12_RESOURCE_STATES newState, ID3D12Resource* resource)
{
	Command& cmd = barrierRecorder[resource];

	if (!&cmd)
	{
#if defined(DEBUG) || defined(_DEBUG)
		throw "Non Initialized Resource!";
#else
		return;
#endif
	}
	if (cmd.index < 0)
	{
		if (cmd.targetState != newState)
		{
			cmd.index = commands.size();
			commands.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
				resource,
				cmd.targetState,
				newState
			));
			cmd.targetState = newState;
		}
	}
	else
	{
		if (commands.empty()) throw "Fuck";
		commands[cmd.index].Transition.StateAfter = newState;
		cmd.targetState = newState;
	}
	

}

void TransitionBarrierBuffer::UAVBarrier(ID3D12Resource* res)
{
	auto ite = uavBarriersDict.Find(res);
	if (!ite)
	{
		uavBarriersDict.Insert(res, true);
		uavBarriers.push_back(res);
	}
}
void TransitionBarrierBuffer::UAVBarriers(const std::initializer_list<ID3D12Resource*>& args)
{
	for (auto i = args.begin(); i != args.end(); ++i)
	{
		UAVBarrier(*i);
	}
}

void TransitionBarrierBuffer::KillSame()
{
	for (size_t i = 0; i < commands.size(); ++i)
	{
		auto& a = commands[i];
		if (a.Transition.StateBefore == a.Transition.StateAfter)
		{
			auto last = commands.end() - 1;
			if (i != (commands.size() - 1))
			{
				a = *last;
			}
			commands.erase(last);
			i--;
		}
		else
		{
			auto uavIte = uavBarriersDict.Find(a.Transition.pResource);
			if (uavIte)
			{
				uavIte.Value() = false;
			}
		}
	}
	D3D12_RESOURCE_BARRIER uavBar;
	uavBar.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	for (auto ite = uavBarriers.begin(); ite != uavBarriers.end(); ++ite)
	{
		uavBar.UAV.pResource = *ite;
		if (uavBarriersDict[uavBar.UAV.pResource])
		{
			commands.push_back(uavBar);
		}
	}
}

void TransitionBarrierBuffer::ExecuteCommand(ID3D12GraphicsCommandList* commandList)
{
	KillSame();
	if (!commands.empty())
	{
		commandList->ResourceBarrier(commands.size(), commands.data());
		commands.clear();
	}
	uavBarriersDict.Clear();
	uavBarriers.clear();
	barrierRecorder.IterateAll([](uint i, ID3D12Resource* key, Command& cmd)->void
		{
			cmd.index = -1;
		});
}

void TransitionBarrierBuffer::Clear(ID3D12GraphicsCommandList* commandList)
{
	KillSame();
	if (!commands.empty())
	{
		commandList->ResourceBarrier(commands.size(), commands.data());
		commands.clear();
	}
	uavBarriersDict.Clear();
	uavBarriers.clear();
	barrierRecorder.Clear();
}

void TransitionBarrierBuffer::AddCommand(D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState, ID3D12Resource* resource)
{
	Command& cmd = barrierRecorder[resource];
	if (!&cmd)
	{
		barrierRecorder.Insert(resource, Command(afterState, commands.size()));
		commands.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			beforeState,
			afterState
		));
	}
	else if (cmd.index < 0)
	{
		cmd.index = commands.size();
		if (cmd.targetState != afterState)
		{
			commands.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
				resource,
				cmd.targetState,
				afterState
			));
			cmd.targetState = afterState;
		}
	}
	else
	{
		commands[cmd.index].Transition.StateAfter = afterState;
		cmd.targetState = afterState;
	}
}