#include "CustomRenderEvent.h"
#include "RenderPipeline.h"
#include "../Singleton/FrameResource.h"
#include "../RenderComponent/TransitionBarrierBuffer.h"
#include "../Common/Camera.h"
#include "../RenderComponent/RenderTexture.h"
CustomRenderEvent::CustomRenderEvent(RenderPipeline* rp) : renderPipeline(rp)
{

}
void CustomRenderEvent::DepthPrepassEvent(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	FrameResource* resource,
	TransitionBarrierBuffer* barrier,
	Camera* cam)
{

}
void CustomRenderEvent::GeometryOpaqueEvent(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	FrameResource* resource,
	TransitionBarrierBuffer* barrier,
	Camera* cam)
{

}
void CustomRenderEvent::TransparentEvent(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* commandList,
	FrameResource* resource,
	TransitionBarrierBuffer* barrier,
	Camera* cam)
{

}