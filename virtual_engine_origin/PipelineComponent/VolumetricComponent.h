#pragma once
#include "PipelineComponent.h"
#include "../RenderComponent/UploadBuffer.h"
class RenderTexture;
class VolumetricComponent : public PipelineComponent
{
private:
	ArrayList<TemporalResourceCommand> tempResource;
public:
	static std::unique_ptr<RenderTexture> volumeRT;
	virtual CommandListType GetCommandListType() {
		return CommandListType::CommandListType_Compute;
	}
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
	virtual ArrayList<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt);
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList);
};

class VolumetricCameraData : public IPipelineResource
{
public:
	std::unique_ptr<RenderTexture> lastVolume;
	UploadBuffer cbuffer;
	VolumetricCameraData(ID3D12Device* device);
	~VolumetricCameraData();
};