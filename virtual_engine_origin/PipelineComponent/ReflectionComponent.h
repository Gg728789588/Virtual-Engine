#pragma once
#include "PipelineComponent.h"
#include "../RenderComponent/RenderComponentInclude.h"
class ReflectionFrameData;
class ReflectionRunnable;
class ReflectionComponent final : public PipelineComponent
{
	friend class ReflectionFrameData;
	friend class ReflectionRunnable;
protected:
	uint TextureIndices;
	uint _CameraGBuffer0;
	uint _CameraGBuffer1;
	uint _CameraGBuffer2;
	uint _Cubemap;
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