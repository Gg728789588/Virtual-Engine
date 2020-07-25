#pragma once
#include "PipelineComponent.h"
class OpaquePostProcess	: public PipelineComponent
{
protected:
	ArrayList<TemporalResourceCommand> tempRT;
	virtual CommandListType GetCommandListType() {
		return CommandListType_Graphics;
	}
	virtual ArrayList<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt);
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList);
public:
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
};