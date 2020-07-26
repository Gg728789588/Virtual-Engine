#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/RenderPackage.h"
#include "../RenderComponent/CBufferPool.h"
#include "../RenderComponent/Utility/CommandSignature.h"
class Shader;
class Mesh;
class Camera;
class StructuredBuffer;
class ComputeShader;

//Cube's drawing system
//Should be 
class CubeDrawer
{
public:
	struct CubeDrawerCopyCommand
	{
		ConstBufferElement sourceCBufferElement;
		uint64_t destOffset;
		uint64_t byteSize;
	};
	
	struct CubeObjectBuffer
	{
		float3 worldPos;
	};

private:
	//Culling Compute Shader's constant buffer
	CBufferPool computeCullBufferPool;
	StackObject<CommandSignature, true> cmdSignature;
	ObjectPtr<Mesh> cubeMesh;
	ObjectPtr<StructuredBuffer> objectDataBuffer;	//0: Object Position 1: Indirect Draw Command
	ObjectPtr<StructuredBuffer> indirectDrawBuffer;
	ObjectPtr<StructuredBuffer> outputPosBuffer;
	uint objectCount = 0;
	ArrayList<CubeDrawerCopyCommand> copyCommands;
	ArrayList<uint> unProcessedRemoveCmd;
	ArrayList<uint2> removeCmdArray;
	Shader const* drawShader;
	ComputeShader const* cullShader;

	std::mutex loadingMtx;
	void DrawPass(RenderPackage const& renderPackage, uint targetPass);
	ID3D12Device* device;
	Math::Vector3 moveDir = { 0,0,0 };
	bool needMove = false;
public:
	Shader const* GetShader() const noexcept
	{
		return drawShader;
	}
	CubeDrawer(ID3D12Device* device);
	ConstBufferElement GetCullConstBuffer(ID3D12Device* device);
	void GetCullConstBuffers(ID3D12Device* device, ConstBufferElement* cbuffers, uint count);
	void GetCullCounstBuffers(ID3D12Device* device, std::initializer_list<ConstBufferElement*> const& cbufferList);

	void ReturnCullConstBuffers(ConstBufferElement* cbuffers, uint count);
	void ReturnCullConstBuffer(ConstBufferElement const& cbuffer);
	void ReturnCullConstBuffers(std::initializer_list<ConstBufferElement*> cbuffers);
	//Logic
	uint AddObject(float3 const& objectPositionMatrix);
	void RemoveObject(uint target);
	//Render Pipeline
	void ExecuteCull(RenderPackage const& renderPackage, ConstBufferElement const& ele, Camera* cam, Math::Vector4* frustumPlanes, Math::Vector3 const& frustumMinPos, Math::Vector3 const& frustumMaxPos);
	void ExecuteCopyCommand(RenderPackage const& renderPackage, Camera* cam);
	void MoveTheWorld(float3 moveDir);
	void DrawDepthPrepass(RenderPackage const& renderPackage)
	{
		DrawPass(renderPackage, 1);
	}
	void DrawGeometry(RenderPackage const& renderPackage)
	{
		DrawPass(renderPackage, 0);
	}
	void DrawDirectionalLightShadow(RenderPackage const& renderPackage)
	{
		DrawPass(renderPackage, 2);
	}
	void DrawSpotLightShadow(RenderPackage const& renderPackage)
	{
		DrawPass(renderPackage, 4);
	}
	void DrawPointLightShadow(RenderPackage const& renderPackage)
	{
		DrawPass(renderPackage, 5);
	}
};