#pragma once
#include "../Singleton/PSOContainer.h"
#include "PipelineComponent.h"
class Texture;
struct GBufferCameraData : public IPipelineResource
{
	UploadBuffer texIndicesBuffer;
	UploadBuffer posBuffer;
	float4x4 vpMatrix;
	GBufferCameraData(ID3D12Device* device);

	~GBufferCameraData();
};
class GeometryComponent final : public PipelineComponent
{
	friend class SkyboxRunnable;
protected:
	ArrayList<TemporalResourceCommand> tempRT;
	virtual CommandListType GetCommandListType() {
		return CommandListType_Graphics;
	}
	virtual ArrayList<TemporalResourceCommand>& SendRenderTextureRequire(const EventData& evt);
	virtual void RenderEvent(const EventData& data, ThreadCommand* commandList);
	
public:
	static UINT _AllLight;
	static UINT _LightIndexBuffer;
	static UINT LightCullCBuffer;
	static UINT TextureIndices;
	static uint _IntegerTex3D;
	static uint _GreyTex;
	static uint _IntegerTex;
	static uint _Cubemap;
	static uint _GreyCubemap;
	static uint _TextureIndirectBuffer;
	static uint DecalParams;
	static uint _AllDecal;
	RenderTexture* preintTexture;
	virtual void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Dispose();
};

