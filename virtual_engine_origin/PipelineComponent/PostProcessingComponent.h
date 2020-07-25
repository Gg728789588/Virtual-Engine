#pragma once
#include "PipelineComponent.h"
#include "PipelineJsonData.h"
#include "../RenderComponent/DescHeapPool.h"
class PostRunnable;
class PrepareComponent;
class PostProcessingComponent : public PipelineComponent
{
	friend class PostRunnable;
protected:
	StackObject<PipelineJsonData> jsonData;
	StackObject<DescHeapPool, true> descPool;
	ArrayList<TemporalResourceCommand> tempRT;
	PrepareComponent* prepareComp;
	virtual CommandListType GetCommandListType() {
		return CommandListType_Graphics;
	}
	virtual ArrayList<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt);
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList);
public:
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
};