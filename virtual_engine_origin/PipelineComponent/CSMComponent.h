#pragma once
#include "PipelineComponent.h"
class CSMComponent : public PipelineComponent
{
private:
	ArrayList<TemporalResourceCommand> tempResources;
public:
	virtual CommandListType GetCommandListType() { return CommandListType_Graphics; }
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
	virtual ArrayList<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt);
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList);
};