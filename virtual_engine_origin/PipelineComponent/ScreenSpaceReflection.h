#pragma once
#include "PipelineComponent.h"
#include "../Singleton/ShaderID.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/PSOContainer.h"
#include "../RenderComponent/RenderComponentInclude.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/ShaderID.h"
#include "../Common/Camera.h"
#include "../Common/RingQueue.h"
#include "../ResourceManagement/AssetDatabase.h"
#include "../CJsonObject/CJsonObject.hpp"
using namespace Math;

struct SSRParameterDescriptor
{
	int NumRays;
	int NumSteps;
	float BRDFBias;
	float Fadeness;
	float RoughnessDiscard;
};

struct SVGFParameterDescriptor
{
	int NumSpatial;
	float SpatialRadius;
	float TemporalScale;
	float TemporalWeight;
	/*float BilateralRadius;
	float BilateralColorWeight;
	float BilateralDepthWeight;
	float BilateralNormalWeight;*/
};

struct SVGFInputDescriptor
{
	Vector4 Resolution;
	Matrix4 Matrix_PrevViewProj;
	Matrix4 Matrix_InvProj;
	Matrix4 Matrix_ViewProj;
	Matrix4 Matrix_InvViewProj;
	Matrix4 Matrix_WorldToView;
	int FrameIndex;
	uint SRV_SceneDepth;
	uint SRV_GBufferMotion;
	uint SRV_GBufferNormal;
	uint SRV_GBufferRoughness;
};
struct SSR_Params_Struct
{
	float4 SSR_TraceResolution;
	float4 SVGF_TemporalSize;
	float4 SVGF_SpatialSize;
	float4 SVGF_BilateralSize;
	uint2 _RayTraceResolution;
	uint2 _PostResolution;
	float SVGF_SpatialRadius;
	float SSR_BRDFBias;
	float SSR_Thickness;
	float SSR_Fadeness;
	float SSR_RoughnessDiscard;
	float ColorPyramidNumLOD;
	float SVGF_TemporalScale;
	float SVGF_TemporalWeight;
	float SVGF_BilateralRadius;
	float SVGF_ColorWeight;
	float SVGF_NormalWeight;
	float SVGF_DepthWeight;
	int SSR_NumRays;
	int SSR_NumSteps;
	int SSR_FrameIndex;
	int SVGF_FrameIndex;
};
struct SSRShaderID
{
	/*
	int NumRays;
	int NumSteps;
	int FrameIndex;
	int BRDFBias;
	int Fadeness;
	int TraceResolution;
	int RoughnessDiscard;

	int Matrix_Proj;
	int Matrix_InvProj;
	int Matrix_InvViewProj;
	int Matrix_WorldToView;*/

	/*  int SRV_Ranking_Tile = ShaderID::PropertyToID("SRV_Ranking_Tile");
	  int SRV_Scrambled_Tile = ShaderID::PropertyToID("SRV_Scrambled_Tile");
	  int SRV_Scrambled_Owen = ShaderID::PropertyToID("SRV_Scrambled_Owen");
	  int SRV_Scrambled_Noise = ShaderID::PropertyToID("SRV_Scrambled_Noise");*/

	int SRV_PyramidColor = ShaderID::PropertyToID("SRV_PyramidColor");
	int SRV_PyramidDepth = ShaderID::PropertyToID("SRV_PyramidDepth");
	int SRV_SceneDepth = ShaderID::PropertyToID("SRV_SceneDepth");
	int SRV_GBufferNormal = ShaderID::PropertyToID("SRV_GBufferNormal");
	int SRV_GBufferRoughness = ShaderID::PropertyToID("SRV_GBufferRoughness");
	int UAV_ReflectionUVWPDF = ShaderID::PropertyToID("UAV_ReflectionUWVPDF");
	int UAV_ReflectionColorMask = ShaderID::PropertyToID("UAV_ReflectionColorMask");
	int SSR_Params = ShaderID::PropertyToID("SSR_Params");

};
struct SVGF_SpatialShaderID
{
	int FrameIndex = ShaderID::PropertyToID("SVGF_FrameIndex");
	int SpatialRadius = ShaderID::PropertyToID("SVGF_SpatialRadius");
	int SpatialSize = ShaderID::PropertyToID("SVGF_SpatialSize");
	int Matrix_InvProj = ShaderID::PropertyToID("Matrix_InvProj");
	int Matrix_InvViewProj = ShaderID::PropertyToID("Matrix_InvViewProj");
	int Matrix_WorldToView = ShaderID::PropertyToID("Matrix_WorldToView");
	int SRV_SceneDepth = ShaderID::PropertyToID("SRV_SceneDepth");
	int SRV_GBufferNormal = ShaderID::PropertyToID("SRV_GBufferNormal");
	int SRV_GBufferRoughness = ShaderID::PropertyToID("SRV_GBufferRoughness");
	int SRV_UWVPDF = ShaderID::PropertyToID("SRV_UWVPDF");
	int SRV_ColorMask = ShaderID::PropertyToID("SRV_ColorMask");
	int UAV_SpatialColor = ShaderID::PropertyToID("UAV_SpatialColor");
};

struct SVGF_TemporalShaderID
{
	int TemporalScale = ShaderID::PropertyToID("SVGF_TemporalScale");
	int TemporalWeight = ShaderID::PropertyToID("SVGF_TemporalWeight");
	int TemporalSize = ShaderID::PropertyToID("SVGF_TemporalSize");
	int Matrix_PrevViewProj = ShaderID::PropertyToID("Matrix_PrevViewProj");
	int Matrix_ViewProj = ShaderID::PropertyToID("Matrix_ViewProj");
	int Matrix_InvViewProj = ShaderID::PropertyToID("Matrix_InvViewProj");
	int SRV_CurrColor = ShaderID::PropertyToID("SRV_CurrColor");
	int SRV_PrevColor = ShaderID::PropertyToID("SRV_PrevColor");
	int SRV_GBufferMotion = ShaderID::PropertyToID("SRV_GBufferMotion");
	int SRV_RayDepth = ShaderID::PropertyToID("SRV_RayDepth");
	int SRV_GBufferNormal = ShaderID::PropertyToID("SRV_GBufferNormal");
	int UAV_TemporalColor = ShaderID::PropertyToID("UAV_TemporalColor");
};

struct SVGF_BilateralShaderID
{
	int BilateralRadius = ShaderID::PropertyToID("SVGF_BilateralRadius");
	int ColorWeight = ShaderID::PropertyToID("SVGF_ColorWeight");
	int NormalWeight = ShaderID::PropertyToID("SVGF_NormalWeight");
	int DepthWeight = ShaderID::PropertyToID("SVGF_DepthWeight");
	int BilateralSize = ShaderID::PropertyToID("SVGF_BilateralSize");
	int SRV_InputColor = ShaderID::PropertyToID("SRV_InputColor");
	int SRV_GBufferNormal = ShaderID::PropertyToID("SRV_GBufferNormal");
	int SRV_SceneDepth = ShaderID::PropertyToID("SRV_SceneDepth");
	int UAV_BilateralColor = ShaderID::PropertyToID("UAV_BilateralColor");
};

class SSRFrameData : public IPipelineResource
{
private:
	CBufferPool* cbPool;
public:
	DescriptorHeap hizDescHeap;
	ConstBufferElement paramEle;
	uint64_t frameIndex = 0;
	SSRFrameData(ID3D12Device* device, CBufferPool* cbPool) :
		hizDescHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 11, true),
		cbPool(cbPool)
	{
		paramEle = cbPool->Get(device);
	}
	~SSRFrameData()
	{
		cbPool->Return(paramEle);
	}
};

class SSRCameraData : public IPipelineResource
{
public:
	std::unique_ptr<RenderTexture> temporalPrev;
};
#define GET_SCREENDEPTH_KERNEL 2
#define FIRST_MIP_KERNEL 0
#define SECOND_MIP_KERNEL 1
#define RAYCAST_KERNEL 3
#define SPATIAL_KERNEL 4
#define TEMPORAL_KERNEL 5
#define BILATERAL_KERNEL 6
class ScreenSpaceReflection
{
private:
	std::mutex mtx;
	inline static const int2 HZBSize = { 1024, 1024 };
	//////////// vars
	const uint sampleRate = 2;
	bool enabled = false;
	////////////
	uint _CameraDepthTexture;
	uint _DepthTexture;
	CBufferPool paramsCBPool;
	StackObject<RenderTexture> RTV_PyramidDepth;
	const ComputeShader* ssrCS;
	const Shader* blitShader;
	SSRShaderID ssrShaderID;
	SVGF_SpatialShaderID svgf_SpatialShaderID;
	SVGF_TemporalShaderID svgf_TemporalShaderID;
	SVGF_BilateralShaderID svgf_BilateralShaderID;
	struct RenderTextures
	{
		RenderTexture* UAV_ReflectionUVWPDF;
		RenderTexture* UAV_ReflectionColorMask;
		RenderTexture* UAV_SpatialColor;
		RenderTexture* UAV_TemporalColor;
	};
	RingQueue<RenderTextures> rQueue;

	ArrayList<uint> tempRTIndices;
	SSRParameterDescriptor ssrSettings;	//TODO: read from json
	SVGFParameterDescriptor svgfSettings;//TODO: read from json
	neb::CJsonObject json;
public:
	ScreenSpaceReflection(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ArrayList<TemporalResourceCommand>& temps, const Shader* blitShader) :
		paramsCBPool(sizeof(SSR_Params_Struct), 12),
		blitShader(blitShader),
		rQueue(3)
	{
		/*ssrSettings.NumRays = 8;
		ssrSettings.NumSteps = 8;
		ssrSettings.BRDFBias = 0.7;
		ssrSettings.Fadeness = 0.1;
		ssrSettings.RoughnessDiscard = 0.5;
		svgfSettings.NumSpatial = 1;
		svgfSettings.SpatialRadius = 2;
		svgfSettings.TemporalScale = 1.25;
		svgfSettings.TemporalWeight = 0.99;
		svgfSettings.BilateralColorWeight = 1;
		svgfSettings.BilateralDepthWeight = 1;
		svgfSettings.BilateralNormalWeight = 0.1;
		svgfSettings.BilateralRadius = 2;*/

		AssetDatabase::GetInstance()->TryGetJson("SSR", &json);
		int enabledData = 0;
		json.Get("enabled", enabledData);
		enabled = enabledData;
		json.Get("NumRays", ssrSettings.NumRays);
		json.Get("NumSteps", ssrSettings.NumSteps);
		json.Get("BRDFBias", ssrSettings.BRDFBias);
		json.Get("Fadeness", ssrSettings.Fadeness);
		json.Get("RoughnessDiscard", ssrSettings.RoughnessDiscard);
		json.Get("NumSpatial", svgfSettings.NumSpatial);
		json.Get("SpatialRadius", svgfSettings.SpatialRadius);
		json.Get("TemporalScale", svgfSettings.TemporalScale);
		json.Get("TemporalWeight", svgfSettings.TemporalWeight);
		tempRTIndices.reserve(10);
		_CameraDepthTexture = ShaderID::PropertyToID("_CameraDepthTexture");
		_DepthTexture = ShaderID::PropertyToID("_DepthTexture");
		TemporalResourceCommand resCommand;
		RenderTextureDescriptor& desc = resCommand.descriptor.rtDesc;
		resCommand.type = TemporalResourceCommand::CommandType_Create_RenderTexture;
		resCommand.uID = ssrShaderID.UAV_ReflectionUVWPDF;
		resCommand.descriptor.type = ResourceDescriptor::ResourceType_RenderTexture;
		desc.depthSlice = 1;
		desc.type = TextureDimension::Tex2D;
		desc.rtFormat = RenderTextureFormat::GetColorFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);
		tempRTIndices.push_back(temps.size());
		temps.push_back(resCommand);
		resCommand.uID = ssrShaderID.UAV_ReflectionColorMask;
		tempRTIndices.push_back(temps.size());
		temps.push_back(resCommand);
		resCommand.uID = svgf_SpatialShaderID.UAV_SpatialColor;
		tempRTIndices.push_back(temps.size());
		temps.push_back(resCommand);
		resCommand.uID = svgf_TemporalShaderID.UAV_TemporalColor;
		tempRTIndices.push_back(temps.size());
		temps.push_back(resCommand);
		ssrCS = ShaderCompiler::GetComputeShader("SSR");
		RTV_PyramidDepth.New(device, 1024, 1024, RenderTextureFormat::GetColorFormat(DXGI_FORMAT_R16_FLOAT), TextureDimension::Tex2D, 1, 10, RenderTextureState::Unordered_Access);

	}

	void PrepareRenderTexture(ArrayList<MObject*>& objs)
	{
		RenderTextures rts;
		rts.UAV_ReflectionUVWPDF = (RenderTexture*)objs[tempRTIndices[0]];
		rts.UAV_ReflectionColorMask = (RenderTexture*)objs[tempRTIndices[1]];
		rts.UAV_SpatialColor = (RenderTexture*)objs[tempRTIndices[2]];
		rts.UAV_TemporalColor = (RenderTexture*)objs[tempRTIndices[3]];
		std::lock_guard<std::mutex> lck(mtx);
		rQueue.Push(rts);
	}

	void UpdateTempRT(const PipelineComponent::EventData& data, ArrayList<TemporalResourceCommand>& temps)
	{
		uint2 screenSize = uint2(data.width, data.height) / uint2(sampleRate, sampleRate);
		temps[tempRTIndices[0]].descriptor.rtDesc.width = screenSize.x;
		temps[tempRTIndices[0]].descriptor.rtDesc.height = screenSize.y;
		temps[tempRTIndices[1]].descriptor.rtDesc.width = screenSize.x;
		temps[tempRTIndices[1]].descriptor.rtDesc.height = screenSize.y;
		temps[tempRTIndices[2]].descriptor.rtDesc.width = data.width;
		temps[tempRTIndices[2]].descriptor.rtDesc.height = data.height;
		temps[tempRTIndices[3]].descriptor.rtDesc.width = data.width;
		temps[tempRTIndices[3]].descriptor.rtDesc.height = data.height;

	}
	~ScreenSpaceReflection()
	{
		RTV_PyramidDepth.Delete();
	}
	void FrameUpdate(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		FrameResource* resource,
		Camera* cam,
		TransitionBarrierBuffer* barrierBuffer,
		RenderTexture* depthTex,
		RenderTexture* gbuffer1,
		RenderTexture* gbuffer2,
		RenderTexture* source,
		RenderTexture* renderTarget,
		RenderTexture* motionVector,
		PSOContainer* toRenderTargetPSOContainer)
	{
		if (!enabled) return;
		RenderTextures rt;
		{
			std::lock_guard<std::mutex> lck(mtx);
			if (!rQueue.TryPop(&rt)) return;
		}
		uint2 screenSize = uint2(renderTarget->GetWidth() / sampleRate, renderTarget->GetHeight() / sampleRate);
		uint2 originScreenSize = uint2(renderTarget->GetWidth(), renderTarget->GetHeight());

		//Depth Pyramid
		SSRFrameData* frameData = (SSRFrameData*)resource->GetPerCameraResource(this, cam, [&]()->SSRFrameData*
			{
				return new SSRFrameData(device, &paramsCBPool);
			});
		SSRCameraData* camData = (SSRCameraData*)cam->GetResource(this, []()->SSRCameraData*
			{
				return new SSRCameraData;
			});
		for (uint i = 0; i < 10; ++i)
		{
			RTV_PyramidDepth->BindUAVToHeap(&frameData->hizDescHeap, i, device, i);
		}
		frameData->frameIndex += 1;
		depthTex->BindSRVToHeap(&frameData->hizDescHeap, 10, device);
		////////////////Mark Barrier Buffer
		barrierBuffer->RegistInitState(RenderTexture::RenderTargetState, renderTarget->GetResource());
		barrierBuffer->RegistInitState(RenderTexture::ColorReadState, motionVector->GetResource());
		barrierBuffer->RegistInitState(RenderTexture::RenderTargetState, rt.UAV_ReflectionUVWPDF->GetResource());
		barrierBuffer->RegistInitState(RenderTexture::RenderTargetState, rt.UAV_ReflectionColorMask->GetResource());
		barrierBuffer->RegistInitState(RenderTexture::RenderTargetState, rt.UAV_SpatialColor->GetResource());
		barrierBuffer->RegistInitState(RenderTexture::RenderTargetState, rt.UAV_TemporalColor->GetResource());
		barrierBuffer->RegistInitState(RenderTexture::UnorderedAccessState, RTV_PyramidDepth->GetResource());


		////////////////
		barrierBuffer->UpdateState(RenderTexture::UnorderedAccessState, rt.UAV_ReflectionUVWPDF->GetResource());
		barrierBuffer->UpdateState(RenderTexture::UnorderedAccessState, rt.UAV_ReflectionColorMask->GetResource());
		barrierBuffer->UpdateState(RenderTexture::UnorderedAccessState, rt.UAV_SpatialColor->GetResource());
		barrierBuffer->UpdateState(RenderTexture::UnorderedAccessState, rt.UAV_TemporalColor->GetResource());
		barrierBuffer->ExecuteCommand(commandList);
		ssrCS->BindRootSignature(commandList, &frameData->hizDescHeap);
		ssrCS->SetResource(commandList, _CameraDepthTexture, &frameData->hizDescHeap, 10);
		ssrCS->SetResource(commandList, _DepthTexture, &frameData->hizDescHeap, 0);
		ssrCS->Dispatch(commandList, GET_SCREENDEPTH_KERNEL, HZBSize.x / 8, HZBSize.y / 8, 1);
		barrierBuffer->UAVBarrier(RTV_PyramidDepth->GetResource());
		barrierBuffer->ExecuteCommand(commandList);
		ssrCS->Dispatch(commandList, FIRST_MIP_KERNEL, HZBSize.x / 32 / 2, HZBSize.y / 32 / 2, 1);
		barrierBuffer->UAVBarrier(RTV_PyramidDepth->GetResource());
		barrierBuffer->ExecuteCommand(commandList);
		ssrCS->Dispatch(commandList, SECOND_MIP_KERNEL, 1, 1, 1);
		barrierBuffer->UpdateState(RenderTexture::ColorReadState, RTV_PyramidDepth->GetResource());
		/////////////////////Ray Casting
		float4 traceResolution;
		traceResolution.x = screenSize.x;
		traceResolution.y = screenSize.y;
		traceResolution.z = 1.0 / traceResolution.x;
		traceResolution.w = 1.0 / traceResolution.y;
		float4 resolution;
		resolution.x = originScreenSize.x;
		resolution.y = originScreenSize.y;
		resolution.z = 1.0 / resolution.x;
		resolution.w = 1.0 / resolution.y;
		SSR_Params_Struct ssrParams;
		ssrParams.SSR_NumRays = ssrSettings.NumRays;
		ssrParams.SSR_NumSteps = ssrSettings.NumSteps;
		ssrParams.SSR_FrameIndex = frameData->frameIndex;
		ssrParams.SSR_BRDFBias = ssrSettings.BRDFBias;
		ssrParams.SSR_Fadeness = ssrSettings.Fadeness;
		ssrParams.SSR_RoughnessDiscard = ssrSettings.RoughnessDiscard;
		ssrParams.SSR_TraceResolution = traceResolution;
		ssrParams._RayTraceResolution = screenSize;
		ssrParams._PostResolution = originScreenSize;
		const DescriptorHeap* globalHeap = Graphics::GetGlobalDescHeap();
		Graphics::GetGlobalDescHeap()->SetDescriptorHeap(commandList);
		ConstBufferElement ele = resource->cameraCBs[cam->GetInstanceID()];

		ssrCS->SetResource(commandList, ShaderID::GetPerCameraBufferID(), ele.buffer, ele.element);
		ssrCS->SetResource(commandList, ssrShaderID.SSR_Params, frameData->paramEle.buffer, frameData->paramEle.element);
		ssrCS->SetResource(commandList, ssrShaderID.SRV_PyramidColor, globalHeap, source->GetGlobalDescIndex());
		ssrCS->SetResource(commandList, ssrShaderID.SRV_SceneDepth, globalHeap, depthTex->GetGlobalDescIndex());
		ssrCS->SetResource(commandList, ssrShaderID.SRV_GBufferNormal, globalHeap, gbuffer2->GetGlobalDescIndex());
		ssrCS->SetResource(commandList, ssrShaderID.SRV_GBufferRoughness, globalHeap, gbuffer1->GetGlobalDescIndex());
		ssrCS->SetResource(commandList, ssrShaderID.SRV_PyramidDepth, globalHeap, RTV_PyramidDepth->GetGlobalDescIndex());
		ssrCS->SetResource(commandList, ssrShaderID.UAV_ReflectionUVWPDF, globalHeap, rt.UAV_ReflectionUVWPDF->GetGlobalUAVDescIndex(0));
		ssrCS->SetResource(commandList, ssrShaderID.UAV_ReflectionColorMask, globalHeap, rt.UAV_ReflectionColorMask->GetGlobalUAVDescIndex(0));
		barrierBuffer->ExecuteCommand(commandList);
		ssrCS->Dispatch(commandList, RAYCAST_KERNEL, (screenSize.x + 15) / 16, (screenSize.y + 15) / 16, 1);
		barrierBuffer->UpdateState(RenderTexture::ColorReadState, rt.UAV_ReflectionUVWPDF->GetResource());

		///////////////////Spatial Filter
		ssrParams.SVGF_FrameIndex = frameData->frameIndex;
		ssrParams.SVGF_SpatialRadius = svgfSettings.SpatialRadius;
		ssrParams.SVGF_SpatialSize = resolution;
		ssrCS->SetResource(commandList, svgf_SpatialShaderID.SRV_UWVPDF, globalHeap, rt.UAV_ReflectionUVWPDF->GetGlobalDescIndex());
		/*for (uint i = 0; i < svgfSettings.NumSpatial; i++) {
			uint CurrState = i & 1;
			if (CurrState == 0)
			{
				barrierBuffer->UpdateState(RenderTexture::ColorReadState, rt.UAV_ReflectionColorMask->GetResource());
				barrierBuffer->UpdateState(RenderTexture::UnorderedAccessState, rt.UAV_SpatialColor->GetResource());
				ssrCS->SetResource(commandList, svgf_SpatialShaderID.SRV_ColorMask, globalHeap, rt.UAV_ReflectionColorMask->GetGlobalDescIndex());
				ssrCS->SetResource(commandList, svgf_SpatialShaderID.UAV_SpatialColor, globalHeap, rt.UAV_SpatialColor->GetGlobalUAVDescIndex(0));
				barrierBuffer->ExecuteCommand(commandList);
				ssrCS->Dispatch(commandList, SPATIAL_KERNEL, (originScreenSize.x + 15) / 16, (originScreenSize.y + 15) / 16, 1);
				if (i == svgfSettings.NumSpatial - 1)
				{
					Graphics::UAVBarriers(commandList, { rt.UAV_SpatialColor->GetResource() });
				}
			}
			else
			{
				barrierBuffer->UpdateState(RenderTexture::ColorReadState, rt.UAV_SpatialColor->GetResource());
				barrierBuffer->UpdateState(RenderTexture::UnorderedAccessState, rt.UAV_ReflectionColorMask->GetResource());
				ssrCS->SetResource(commandList, svgf_SpatialShaderID.SRV_ColorMask, globalHeap, rt.UAV_SpatialColor->GetGlobalDescIndex());
				ssrCS->SetResource(commandList, svgf_SpatialShaderID.UAV_SpatialColor, globalHeap, rt.UAV_ReflectionColorMask->GetGlobalUAVDescIndex(0));
				barrierBuffer->ExecuteCommand(commandList);
				ssrCS->Dispatch(commandList, SPATIAL_KERNEL, (originScreenSize.x + 15) / 16, (originScreenSize.y + 15) / 16, 1);
				barrierBuffer->UpdateState(RenderTexture::ColorReadState, rt.UAV_ReflectionColorMask->GetResource());
				barrierBuffer->UpdateState(RenderTexture::CopyDestState, rt.UAV_SpatialColor->GetResource());
				barrierBuffer->ExecuteCommand(commandList);
				Graphics::CopyTexture(commandList, rt.UAV_ReflectionColorMask, 0, 0, rt.UAV_SpatialColor, 0, 0);
			}

		}*/
		barrierBuffer->UpdateState(RenderTexture::ColorReadState, rt.UAV_ReflectionColorMask->GetResource());
		barrierBuffer->UpdateState(RenderTexture::UnorderedAccessState, rt.UAV_SpatialColor->GetResource());
		ssrCS->SetResource(commandList, svgf_SpatialShaderID.SRV_ColorMask, globalHeap, rt.UAV_ReflectionColorMask->GetGlobalDescIndex());
		ssrCS->SetResource(commandList, svgf_SpatialShaderID.UAV_SpatialColor, globalHeap, rt.UAV_SpatialColor->GetGlobalUAVDescIndex(0));
		barrierBuffer->ExecuteCommand(commandList);
		ssrCS->Dispatch(commandList, SPATIAL_KERNEL, (originScreenSize.x + 15) / 16, (originScreenSize.y + 15) / 16, 1);
		barrierBuffer->UAVBarrier(rt.UAV_SpatialColor->GetResource());
		////////////////////Temporal Filter
		if (!camData->temporalPrev
			|| (camData->temporalPrev->GetWidth() != originScreenSize.x)
			|| (camData->temporalPrev->GetHeight() != originScreenSize.y))
		{
			camData->temporalPrev = std::unique_ptr<RenderTexture>(new RenderTexture(
				device,
				originScreenSize.x,
				originScreenSize.y,
				RenderTextureFormat::GetColorFormat(rt.UAV_TemporalColor->GetFormat()),
				TextureDimension::Tex2D,
				1, 1, RenderTextureState::Unordered_Access));
		}
		barrierBuffer->RegistInitState(RenderTexture::UnorderedAccessState, camData->temporalPrev->GetResource());
		ssrParams.SVGF_TemporalScale = svgfSettings.TemporalScale;
		ssrParams.SVGF_TemporalWeight = svgfSettings.TemporalWeight;
		ssrParams.SVGF_TemporalSize = resolution;
		barrierBuffer->UpdateState(RenderTexture::ColorReadState, rt.UAV_ReflectionUVWPDF->GetResource());
		barrierBuffer->UpdateState(RenderTexture::ColorReadState, rt.UAV_SpatialColor->GetResource());
		barrierBuffer->UpdateState(RenderTexture::ColorReadState, camData->temporalPrev->GetResource());
		barrierBuffer->UpdateState(RenderTexture::UnorderedAccessState, rt.UAV_TemporalColor->GetResource());
		ssrCS->SetResource(commandList, svgf_TemporalShaderID.SRV_GBufferMotion, globalHeap, motionVector->GetGlobalDescIndex());
		ssrCS->SetResource(commandList, svgf_TemporalShaderID.SRV_RayDepth, globalHeap, rt.UAV_ReflectionUVWPDF->GetGlobalDescIndex());
		ssrCS->SetResource(commandList, svgf_TemporalShaderID.SRV_CurrColor, globalHeap, rt.UAV_SpatialColor->GetGlobalDescIndex());
		ssrCS->SetResource(commandList, svgf_TemporalShaderID.SRV_PrevColor, globalHeap, camData->temporalPrev->GetGlobalDescIndex());
		ssrCS->SetResource(commandList, svgf_TemporalShaderID.UAV_TemporalColor, globalHeap, rt.UAV_TemporalColor->GetGlobalUAVDescIndex(0));
		barrierBuffer->ExecuteCommand(commandList);
		ssrCS->Dispatch(commandList, TEMPORAL_KERNEL, (15 + originScreenSize.x) / 16, (15 + originScreenSize.y) / 16, 1);
		barrierBuffer->UpdateState(RenderTexture::ColorReadState, rt.UAV_TemporalColor->GetResource());
		barrierBuffer->UpdateState(RenderTexture::CopyDestState, camData->temporalPrev->GetResource());
		barrierBuffer->ExecuteCommand(commandList);
		Graphics::CopyTexture(commandList, rt.UAV_TemporalColor, 0, 0, camData->temporalPrev.get(), 0, 0);

		barrierBuffer->ExecuteCommand(commandList);//Delete
		//////////////Blit
		blitShader->BindRootSignature(commandList, globalHeap);
		blitShader->SetResource(commandList, ShaderID::GetMainTex(), globalHeap, rt.UAV_TemporalColor->GetGlobalDescIndex());
		blitShader->SetResource(commandList, ssrShaderID.SRV_GBufferRoughness, globalHeap, gbuffer1->GetGlobalDescIndex());
		Graphics::Blit(commandList,
			device,
			{ RenderTarget(renderTarget) },
			RenderTarget(),
			toRenderTargetPSOContainer,
			blitShader,
			0);
		barrierBuffer->UpdateState(RenderTexture::RenderTargetState, rt.UAV_ReflectionUVWPDF->GetResource());
		barrierBuffer->UpdateState(RenderTexture::RenderTargetState, rt.UAV_ReflectionColorMask->GetResource());
		barrierBuffer->UpdateState(RenderTexture::RenderTargetState, rt.UAV_SpatialColor->GetResource());
		barrierBuffer->UpdateState(RenderTexture::RenderTargetState, rt.UAV_TemporalColor->GetResource());
		barrierBuffer->UpdateState(RenderTexture::UnorderedAccessState, RTV_PyramidDepth->GetResource());
		barrierBuffer->UpdateState(RenderTexture::UnorderedAccessState, camData->temporalPrev->GetResource());
		frameData->paramEle.buffer->CopyData(frameData->paramEle.element, &ssrParams);
	}
};

#undef GET_SCREENDEPTH_KERNEL
#undef FIRST_MIP_KERNEL
#undef SECOND_MIP_KERNEL
#undef RAYCAST_KERNEL
#undef SPATIAL_KERNEL
#undef TEMPORAL_KERNEL
#undef BILATERAL_KERNEL