#include "CubeDrawer.h"
#include "../RenderComponent/RenderComponentInclude.h"
#include "../Singleton/ShaderCompiler.h"
#include "../Singleton/PSOContainer.h"
#include "../Singleton/ShaderID.h"
#include "../PipelineComponent/IPipelineResource.h"
#include "../Singleton/FrameResource.h"
#include "../Common/Camera.h"
using namespace Math;
struct CullDispatchParams
{
	float4 frustumPlanes[6];
	float4 frustumMinPoint;
	float4 frustumMaxPoint;
	uint _Count;
};
struct CubeMoveTheWorldParams
{
	float3 _MoveDir;
	uint _MoveCount;
};
struct CubeDrawerPerCameraData : public IPipelineResource
{
	ArrayList<ConstBufferElement> copyCommands;
	CBufferPool* pool;
	ObjectPtr<UploadBuffer> removeCmd;
	CubeDrawerPerCameraData(CBufferPool* pool, ID3D12Device* device) :
		pool(pool)
	{
	}
	void Clear()
	{
		for (auto ite = copyCommands.begin(); ite != copyCommands.end(); ++ite)
		{
			pool->Return(*ite);
		}
		copyCommands.clear();
	}
	~CubeDrawerPerCameraData()
	{
	}
};
namespace CubeDrawerGlobal
{
	//Shader Property ID
	constexpr uint MAXIMUM_CUBE_COUNT = 500000;
	uint _PerObjectData;
	uint _InputDataBuffer;
	uint CullBuffer;
	uint _OutputBuffer;
	uint _OutputDataBuffer;
	uint _RemoveBuffer;
	uint MoveTheWorld;
}

CubeDrawer::CubeDrawer(ID3D12Device* device) :
	computeCullBufferPool(
		Max(sizeof(CubeObjectBuffer),
			Max(sizeof(InstanceIndirectCommand),
				Max(sizeof(CubeMoveTheWorldParams), sizeof(CullDispatchParams)))), 256, true)
{
	drawShader = ShaderCompiler::GetShader("OpaqueStandard");
	cullShader = ShaderCompiler::GetComputeShader("Cull");
	cmdSignature.New(device, CommandSignature::SignatureType::DrawInstanceIndirect, drawShader);
	this->device = device;
	Mesh::LoadMeshFromFile(
		cubeMesh, "Resource/Internal/Cube.vmesh",
		device,
		true,
		true,
		false,
		true,
		false,
		false,
		false,
		false);
	objectDataBuffer = ObjectPtr<StructuredBuffer>::NewObject(
		device,
		std::initializer_list< StructuredBufferElement>{
		StructuredBufferElement::Get(sizeof(CubeObjectBuffer), CubeDrawerGlobal::MAXIMUM_CUBE_COUNT)},
		D3D12_RESOURCE_STATE_GENERIC_READ);
	indirectDrawBuffer = ObjectPtr<StructuredBuffer>::NewObject(
		device,
		std::initializer_list< StructuredBufferElement>
	{
		StructuredBufferElement::Get(sizeof(InstanceIndirectCommand), 1)
	},
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	outputPosBuffer = ObjectPtr<StructuredBuffer>::NewObject(
		device,
		std::initializer_list< StructuredBufferElement>
	{
		StructuredBufferElement::Get(sizeof(CubeObjectBuffer), CubeDrawerGlobal::MAXIMUM_CUBE_COUNT)
	},
		D3D12_RESOURCE_STATE_GENERIC_READ
			);
	CubeDrawerGlobal::_PerObjectData = ShaderID::PropertyToID("_PerObjectData");
	CubeDrawerGlobal::_InputDataBuffer = ShaderID::PropertyToID("_InputDataBuffer");
	CubeDrawerGlobal::CullBuffer = ShaderID::PropertyToID("CullBuffer");
	CubeDrawerGlobal::_OutputBuffer = ShaderID::PropertyToID("_OutputBuffer");
	CubeDrawerGlobal::_OutputDataBuffer = ShaderID::PropertyToID("_OutputDataBuffer");
	CubeDrawerGlobal::_RemoveBuffer = ShaderID::PropertyToID("_RemoveBuffer");
	CubeDrawerGlobal::MoveTheWorld = ShaderID::PropertyToID("MoveTheWorld");
	//TODO
	//Test
	float3 pos = { 0,0,0 };
	AddObject(pos);
	pos.y = 2;
	AddObject(pos);
	pos.y = 4;
	AddObject(pos);
	//RemoveObject(0);
}

ConstBufferElement CubeDrawer::GetCullConstBuffer(ID3D12Device* device)
{
	return computeCullBufferPool.Get(device);
}
void CubeDrawer::GetCullConstBuffers(ID3D12Device* device, ConstBufferElement* cbuffers, uint count)
{
	for (uint i = 0; i < count; ++i)
	{
		cbuffers[i] = computeCullBufferPool.Get(device);
	}
}
void CubeDrawer::GetCullCounstBuffers(ID3D12Device* device, std::initializer_list<ConstBufferElement*> const& cbufferList)
{
	for (auto ite = cbufferList.begin(); ite != cbufferList.end(); ++ite)
	{
		**ite = computeCullBufferPool.Get(device);
	}
}

void CubeDrawer::ReturnCullConstBuffers(ConstBufferElement* cbuffers, uint count)
{
	for (uint i = 0; i < count; ++i)
	{
		computeCullBufferPool.Return(cbuffers[i]);
	}
}
void CubeDrawer::ReturnCullConstBuffer(ConstBufferElement const& cbuffer)
{
	computeCullBufferPool.Return(cbuffer);
}
void CubeDrawer::ReturnCullConstBuffers(std::initializer_list<ConstBufferElement*> cbufferList)
{
	for (auto ite = cbufferList.begin(); ite != cbufferList.end(); ++ite)
	{
		computeCullBufferPool.Return(**ite);
	}
}
uint CubeDrawer::AddObject(float3 const& objPos)
{
	ConstBufferElement ele = computeCullBufferPool.Get(device);
	CubeObjectBuffer eleMappedPtr;
	eleMappedPtr.worldPos = objPos;
	memcpy(ele.buffer->GetMappedDataPtr(ele.element), &eleMappedPtr, sizeof(CubeObjectBuffer));
	CubeDrawerCopyCommand copyCmd;
	copyCmd.sourceCBufferElement = ele;
	copyCmd.byteSize = sizeof(CubeObjectBuffer);
	copyCmd.destOffset = objectDataBuffer->GetAddressOffset(0, objectCount);
	copyCommands.push_back(copyCmd);
	uint lastCount = objectCount++;

	return lastCount;
}
void CubeDrawer::RemoveObject(uint target)
{
	objectCount--;
	unProcessedRemoveCmd.push_back({ target });

	/*auto last = objectDatas.end() - 1;
	ConstBufferElement ele = computeCullBufferPool.Get(device);
	CubeObjectBuffer& lastData = *last;
	memcpy(ele.buffer->GetMappedDataPtr(ele.element), &lastData, sizeof(CubeObjectBuffer));
	objectDatas[target] = lastData;
	objectDatas.erase(last);
	CubeDrawerCopyCommand copyCmd;
	copyCmd.sourceCBufferElement = ele;
	copyCmd.byteSize = sizeof(CubeObjectBuffer);
	copyCmd.destOffset = objectDataBuffer->GetAddressOffset(0, target);
	copyCommands.push_back(copyCmd);*/

	//TODO
	//Remove Command
}

void CubeDrawer::ExecuteCopyCommand(RenderPackage const& renderPackage, Camera* cam)
{
	CubeDrawerPerCameraData* frameData = (CubeDrawerPerCameraData*)renderPackage.frameRes->GetPerCameraResource(
		this, cam, [&]()->CubeDrawerPerCameraData* {
			return new CubeDrawerPerCameraData(&computeCullBufferPool, renderPackage.device);
		});
	auto commandList = renderPackage.commandList;
	renderPackage.transitionBarrier->AddCommand(objectDataBuffer->GetInitState(), D3D12_RESOURCE_STATE_COPY_DEST, objectDataBuffer->GetResource());
	renderPackage.transitionBarrier->ExecuteCommand(commandList);
	frameData->Clear();
	for (auto ite = copyCommands.begin(); ite != copyCommands.end(); ++ite)
	{
		frameData->copyCommands.push_back(ite->sourceCBufferElement);
		commandList->CopyBufferRegion(
			objectDataBuffer->GetResource(),
			ite->destOffset,
			ite->sourceCBufferElement.buffer->GetResource(),
			ite->sourceCBufferElement.buffer->GetAddressOffset(ite->sourceCBufferElement.element),
			ite->byteSize
		);
	}
	copyCommands.clear();
	if (!unProcessedRemoveCmd.empty())
	{
		uint lastElement = objectCount + unProcessedRemoveCmd.size() - 1;
		for (auto ite = unProcessedRemoveCmd.begin(); ite != unProcessedRemoveCmd.end(); ++ite)
		{
			uint value = *ite;
			if (value < objectCount)
			{
				removeCmdArray.push_back({ value, lastElement });
			}
			lastElement--;
		}
		unProcessedRemoveCmd.clear();
	}
	if (!removeCmdArray.empty())
	{
		renderPackage.transitionBarrier->UpdateState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, objectDataBuffer->GetResource());
		renderPackage.transitionBarrier->ExecuteCommand(commandList);
		if (!frameData->removeCmd)
		{
			frameData->removeCmd = ObjectPtr<UploadBuffer>::NewObject(device, Max<uint>(256, removeCmdArray.size()), false, sizeof(uint2));
		}
		{
			uint currentSize = frameData->removeCmd->GetElementCount();
			while (currentSize < removeCmdArray.size())
			{
				currentSize *= 2;
			}
			frameData->removeCmd = ObjectPtr<UploadBuffer>::NewObject(device, currentSize, false, sizeof(uint2));
			memcpy(frameData->removeCmd->GetMappedDataPtr(0), removeCmdArray.data(), removeCmdArray.size() * sizeof(uint2));
		}
		ConstBufferElement paramEle = computeCullBufferPool.Get(device);
		frameData->copyCommands.push_back(paramEle);
		CullDispatchParams* data = (CullDispatchParams*)paramEle.buffer->GetMappedDataPtr(paramEle.element);
		data->_Count = removeCmdArray.size();
		//TODO
		//Dispatch
		cullShader->BindRootSignature(commandList);
		cullShader->SetResource(commandList, CubeDrawerGlobal::CullBuffer, paramEle.buffer, paramEle.element);
		cullShader->SetResource<StructuredBuffer>(commandList, CubeDrawerGlobal::_OutputDataBuffer, objectDataBuffer, 0);
		cullShader->SetResource<UploadBuffer>(commandList, CubeDrawerGlobal::_RemoveBuffer, frameData->removeCmd, 0);
		cullShader->Dispatch(commandList, 2, (removeCmdArray.size() + 63) / 64, 1, 1);
		removeCmdArray.clear();
	}
	if (needMove)
	{
		needMove = false;
		renderPackage.transitionBarrier->UAVBarrier(objectDataBuffer->GetResource());
		renderPackage.transitionBarrier->UpdateState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, objectDataBuffer->GetResource());
		renderPackage.transitionBarrier->ExecuteCommand(commandList);
		CubeMoveTheWorldParams params;
		params._MoveCount = objectCount;
		params._MoveDir = moveDir;
		ConstBufferElement paramEle = computeCullBufferPool.Get(device);
		frameData->copyCommands.push_back(paramEle);
		memcpy(paramEle.buffer->GetMappedDataPtr(paramEle.element), &params, sizeof(CubeMoveTheWorldParams));
		cullShader->BindRootSignature(commandList);
		cullShader->SetResource(commandList, CubeDrawerGlobal::MoveTheWorld, paramEle.buffer, paramEle.element);
		cullShader->SetResource<StructuredBuffer>(commandList, CubeDrawerGlobal::_OutputDataBuffer, objectDataBuffer, 0);
		cullShader->Dispatch(commandList, 3, (63 + objectCount) / 64, 1, 1);
		moveDir = Vector3(0);
	}
	renderPackage.transitionBarrier->UpdateState(objectDataBuffer->GetInitState(), objectDataBuffer->GetResource());
}

void CubeDrawer::ExecuteCull(
	RenderPackage const& renderPackage,
	ConstBufferElement const& ele,
	Camera* cam,
	Math::Vector4* frustumPlanes,
	Math::Vector3 const& frustumMinPos,
	Math::Vector3 const& frustumMaxPos)
{
	CubeDrawerPerCameraData* frameData = (CubeDrawerPerCameraData*)renderPackage.frameRes->GetPerCameraResource(
		this, cam, [&]()->CubeDrawerPerCameraData* {
			return new CubeDrawerPerCameraData(&computeCullBufferPool, renderPackage.device);
		});
	CullDispatchParams cullData;
	cullData.frustumMaxPoint = frustumMaxPos;
	cullData.frustumMinPoint = frustumMinPos;
	memcpy(cullData.frustumPlanes, frustumPlanes, sizeof(float4) * 6);
	cullData._Count = objectCount;
	memcpy(
		ele.buffer->GetMappedDataPtr(ele.element),
		&cullData,
		sizeof(CullDispatchParams)
	);
	auto commandList = renderPackage.commandList;
	cullShader->BindRootSignature(commandList);
	cullShader->SetResource(
		commandList,
		CubeDrawerGlobal::CullBuffer,
		ele.buffer,
		ele.element
	);
	cullShader->SetStructuredBufferByAddress(
		commandList,
		CubeDrawerGlobal::_InputDataBuffer,
		objectDataBuffer->GetAddress(0, 0));
	cullShader->SetStructuredBufferByAddress(
		commandList,
		CubeDrawerGlobal::_OutputBuffer,
		indirectDrawBuffer->GetAddress(0, 0)
	);
	cullShader->SetStructuredBufferByAddress(
		commandList,
		CubeDrawerGlobal::_OutputDataBuffer,
		outputPosBuffer->GetAddress(0, 0)
	);
	renderPackage.transitionBarrier->RegistInitState(indirectDrawBuffer->GetInitState(), indirectDrawBuffer->GetResource());
	renderPackage.transitionBarrier->RegistInitState(outputPosBuffer->GetInitState(), outputPosBuffer->GetResource());
	renderPackage.transitionBarrier->UpdateState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, indirectDrawBuffer->GetResource());
	renderPackage.transitionBarrier->UpdateState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS, outputPosBuffer->GetResource());
	renderPackage.transitionBarrier->ExecuteCommand(commandList);
	cullShader->Dispatch(commandList, 1, 1, 1, 1);
	if (objectCount > 0)
	{
		renderPackage.transitionBarrier->UAVBarrier(indirectDrawBuffer->GetResource());
		renderPackage.transitionBarrier->ExecuteCommand(commandList);
		cullShader->Dispatch(commandList, 0, (63 + objectCount) / 64, 1, 1);
	}
	renderPackage.transitionBarrier->UpdateState(indirectDrawBuffer->GetInitState(), indirectDrawBuffer->GetResource());
	renderPackage.transitionBarrier->UpdateState(outputPosBuffer->GetInitState(), outputPosBuffer->GetResource());
}

void CubeDrawer::DrawPass(RenderPackage const& renderPackage, uint targetPass)
{
	Mesh* mesh = cubeMesh;
	auto commandList = renderPackage.commandList;
	PSODescriptor desc;
	desc.meshLayoutIndex = cubeMesh->GetLayoutIndex();
	desc.shaderPass = targetPass;
	desc.shaderPtr = drawShader;
	ID3D12PipelineState* pso = renderPackage.psoContainer->GetState(desc, renderPackage.device);
	//
	/*commandList->SetPipelineState(pso);
	commandList->IASetVertexBuffers(0, 1, &mesh->VertexBufferView());
	commandList->IASetIndexBuffer(&mesh->IndexBufferView());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
	*/
	renderPackage.transitionBarrier->ExecuteCommand(commandList);
	drawShader->SetStructuredBufferByAddress(commandList, CubeDrawerGlobal::_PerObjectData, outputPosBuffer->GetAddress(0, 0));
	commandList->SetPipelineState(pso);
	commandList->IASetVertexBuffers(0, 1, &mesh->VertexBufferView());
	commandList->IASetIndexBuffer(&mesh->IndexBufferView());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->ExecuteIndirect(
		cmdSignature->GetSignature(),
		1,
		indirectDrawBuffer->GetResource(),
		indirectDrawBuffer->GetAddressOffset(0, 0),
		nullptr, 0);
}

void CubeDrawer::MoveTheWorld(float3 moveDir)
{
	this->moveDir += Vector3(moveDir);
	needMove = true;
}