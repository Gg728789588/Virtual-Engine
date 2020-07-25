#pragma once
#include "PipelineComponent.h"
#include "../Utility/PassConstants.h"
class Camera;
class PrepareRunnable;
class MeshRenderer;
class PrepareComponent : public PipelineComponent
{
	friend class PrepareRunnable;
public:
	Math::Vector4 frustumPlanes[6];
	PassConstants passConstants;
	
	Math::Vector3 frustumMinPos;
	Math::Vector3 frustumMaxPos;
	DirectX::XMFLOAT4 _ZBufferParams;
protected:
	ArrayList<TemporalResourceCommand> useless;
	virtual CommandListType GetCommandListType() {
		return CommandListType_None;
	}
	virtual ArrayList<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt) { return useless; }
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList);
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {

	};
	virtual void Dispose() {};
};
