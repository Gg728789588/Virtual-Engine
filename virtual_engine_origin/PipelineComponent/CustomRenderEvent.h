#pragma once
#include "../Common/d3dUtil.h"
class FrameResource;
class TransitionBarrierBuffer;
class RenderPipeline;
class Camera;
class RenderTexture;
class CustomRenderEvent
{
private:
	RenderPipeline* renderPipeline;
public:
	CustomRenderEvent(RenderPipeline* rp);
	void DepthPrepassEvent(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource,
		TransitionBarrierBuffer* barrier,
		Camera* cam);
	void GeometryOpaqueEvent(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource,
		TransitionBarrierBuffer* barrier,
		Camera* cam);
	void TransparentEvent(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource,
		TransitionBarrierBuffer* barrier,
		Camera* cam);
};