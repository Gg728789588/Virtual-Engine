#pragma once
#include "d3dUtil.h"
class FrameResource;
class TransitionBarrierBuffer;
class PSOContainer;
struct RenderPackage
{
	ID3D12Device* const device;
	ID3D12GraphicsCommandList* const  commandList;
	FrameResource* const  frameRes;
	TransitionBarrierBuffer* const transitionBarrier;
	PSOContainer* const psoContainer;
	constexpr RenderPackage(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* frameRes,
		TransitionBarrierBuffer* transitionBarrier,
		PSOContainer* psoContainer) :
		device(device),
		commandList(commandList),
		frameRes(frameRes),
		transitionBarrier(transitionBarrier),
		psoContainer(psoContainer) {}

	constexpr RenderPackage() : device(nullptr),
		commandList(nullptr),
		frameRes(nullptr),
		transitionBarrier(nullptr),
		psoContainer(nullptr)
	{
		
	}
	constexpr RenderPackage(const RenderPackage& p):
		device(p.device),
		commandList(p.commandList),
		frameRes(p.frameRes),
		transitionBarrier(p.transitionBarrier),
		psoContainer(p.psoContainer)
	{
	}

	constexpr bool operator==(const RenderPackage& p) const
	{
		return
			device == p.device &&
			commandList == p.commandList &&
			frameRes == p.frameRes &&
			transitionBarrier == p.transitionBarrier &&
			psoContainer == p.psoContainer;
	}
	constexpr bool operator!=(const RenderPackage& p) const
	{
		return !operator==(p);
	}
};