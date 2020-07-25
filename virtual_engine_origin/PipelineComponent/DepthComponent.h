#pragma once 
#include "PipelineComponent.h"
class RenderTexture;
class UploadBuffer;
class DescriptorHeap;
class PSOContainer;
class DepthComponent : public PipelineComponent
{
private:

public:
	PSOContainer* depthPrepassContainer = nullptr;
	ArrayList<TemporalResourceCommand> tempRTRequire;
	virtual CommandListType GetCommandListType()
	{
		return CommandListType_Graphics;
	}
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
	virtual ArrayList<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt);
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList);
};