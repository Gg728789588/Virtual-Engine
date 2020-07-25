#pragma once
#include "PipelineComponent.h"
#include "../RenderComponent/RenderComponentInclude.h"
#include "../RenderComponent/Light.h"
class PrepareComponent;
class Light;
class LightFrameData : public IPipelineResource
{
public:
	UploadBuffer lightsInFrustum;
	
	LightFrameData(ID3D12Device* device);
};
class LightCameraData : public IPipelineResource
{
public:
	UploadBuffer lightCBuffer;
	LightCameraData(ID3D12Device* device);
};

class LightingComponent : public PipelineComponent
{
private:
	ArrayList<TemporalResourceCommand> tempResources;
	PrepareComponent* prepareComp;
public:
	StackObject<RenderTexture> xyPlaneTexture;
	StackObject<RenderTexture> zPlaneTexture;
	StackObject<StructuredBuffer> lightIndexBuffer;
	StackObject<DescriptorHeap> cullingDescHeap;
	ArrayList<LightCommand> lights;
	ArrayList<Light*> lightPtrs;
	virtual CommandListType GetCommandListType() { return CommandListType_Compute; }
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
	virtual ArrayList<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt);
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList);
};