#include "ComputeShader.h"
#include "../Singleton/ShaderID.h"
#include "UploadBuffer.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "../Common/d3dUtil.h"
#include <fstream>
#include "StructuredBuffer.h"
#include "../JobSystem/JobInclude.h"
#include "Utility/ShaderIO.h"

using Microsoft::WRL::ComPtr;
struct ComputeShaderCompiler
{
	ComputeShader* shader;
	UINT kernelSize;
	vengine::string kernelName;
	ID3D12Device* device;
	vengine::string compilePath;
	UINT i;
	void operator()()
	{

		shader->csShaders[i] = d3dUtil::CompileShader(compilePath, nullptr, kernelName, "cs_5_1");//Compile(compilePath, kernelName[i], nullptr, useCache);
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = shader->mRootSignature.Get();
		psoDesc.CS =
		{
			reinterpret_cast<BYTE*>(shader->csShaders[i]->GetBufferPointer()),
			shader->csShaders[i]->GetBufferSize()
		};
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&shader->pso[i])));

	}
};

ComputeShader::ComputeShader(
	const vengine::string& csoFilePath,
	ID3D12Device* device) : cmdSig(device, CommandSignature::SignatureType::DispatchComputeIndirect)
{
	ShaderIO::DecodeComputeShader(csoFilePath, mVariablesVector, csShaders);
	mVariablesDict.Reserve(mVariablesVector.size() + 2);
	for (int i = 0; i < mVariablesVector.size(); ++i)
	{
		ComputeShaderVariable& variable = mVariablesVector[i];
		mVariablesDict.Insert(ShaderID::PropertyToID(variable.name), i);
	}
	ArrayList<CD3DX12_ROOT_PARAMETER> allParameter;
	auto staticSamplers = d3dUtil::GetStaticSamplers();
	allParameter.reserve(VariableLength());
	ArrayList< CD3DX12_DESCRIPTOR_RANGE> allTexTable;
	IterateVariables([&](ComputeShaderVariable& var) -> void {
		if (var.type == ComputeShaderVariable::Type::SRVDescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
		else if (var.type == ComputeShaderVariable::Type::UAVDescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
		});
	UINT offset = 0;
	IterateVariables([&](ComputeShaderVariable& var) -> void
		{
			CD3DX12_ROOT_PARAMETER slotRootParameter;
			switch (var.type)
			{
			case ComputeShaderVariable::Type::SRVDescriptorHeap:
				slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
				offset++;
				break;
			case ComputeShaderVariable::Type::UAVDescriptorHeap:
				slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
				offset++;
				break;
			case ComputeShaderVariable::Type::ConstantBuffer:
				slotRootParameter.InitAsConstantBufferView(var.registerPos, var.space);
				break;
			case ComputeShaderVariable::Type::StructuredBuffer:
				slotRootParameter.InitAsShaderResourceView(var.registerPos, var.space);
				break;
			case ComputeShaderVariable::Type::RWStructuredBuffer:
				slotRootParameter.InitAsUnorderedAccessView(var.registerPos, var.space);
				break;
			default:
				return;
			}
			allParameter.push_back(slotRootParameter);
		});
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(allParameter.size(), allParameter.data(),
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
	pso.resize(csShaders.size());
	for (int i = 0; i < csShaders.size(); ++i)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.CS =
		{
			reinterpret_cast<BYTE*>(csShaders[i]->GetBufferPointer()),
			csShaders[i]->GetBufferSize()
		};
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso[i])));
	}
}

ComputeShader::ComputeShader(
	const vengine::string& compilePath,
	vengine::string* kernelName, UINT kernelCount,
	ComputeShaderVariable* allShaderVariables, UINT varSize,
	ID3D12Device* device,
	JobBucket* compileJob) : csShaders(kernelCount), pso(kernelCount),
	mVariablesDict(varSize + 2), cmdSig(device, CommandSignature::SignatureType::DispatchComputeIndirect)
{
	mVariablesVector.reserve(varSize);
	for (int i = 0; i < varSize; ++i)
	{
		ComputeShaderVariable& variable = allShaderVariables[i];
		mVariablesDict.Insert(ShaderID::PropertyToID(variable.name), i);
		mVariablesVector.push_back(variable);

	}

	ArrayList<CD3DX12_ROOT_PARAMETER> allParameter;
	auto staticSamplers = d3dUtil::GetStaticSamplers();
	allParameter.reserve(VariableLength());
	ArrayList< CD3DX12_DESCRIPTOR_RANGE> allTexTable;
	IterateVariables([&](ComputeShaderVariable& var) -> void {
		if (var.type == ComputeShaderVariable::Type::SRVDescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
		else if (var.type == ComputeShaderVariable::Type::UAVDescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
		});
	UINT offset = 0;
	IterateVariables([&](ComputeShaderVariable& var) -> void
		{
			CD3DX12_ROOT_PARAMETER slotRootParameter;
			switch (var.type)
			{
			case ComputeShaderVariable::Type::SRVDescriptorHeap:
				slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
				offset++;
				break;
			case ComputeShaderVariable::Type::UAVDescriptorHeap:
				slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
				offset++;
				break;
			case ComputeShaderVariable::Type::ConstantBuffer:
				slotRootParameter.InitAsConstantBufferView(var.registerPos, var.space);
				break;
			case ComputeShaderVariable::Type::StructuredBuffer:
				slotRootParameter.InitAsShaderResourceView(var.registerPos, var.space);
				break;
			case ComputeShaderVariable::Type::RWStructuredBuffer:
				slotRootParameter.InitAsUnorderedAccessView(var.registerPos, var.space);
				break;
			default:
				return;
			}
			allParameter.push_back(slotRootParameter);
		});
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(allParameter.size(), allParameter.data(),
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
	UINT kernelSize = kernelCount;
	if (compileJob != nullptr)
	{
		for (UINT i = 0; i < kernelSize; ++i)
		{
			compileJob->GetTask<ComputeShaderCompiler>(nullptr, 0,
				{
					this,
					kernelSize,
					kernelName[i],
					device,
					compilePath,
					i
				});
		}
	}
	else
	{
		for (int i = 0; i < kernelSize; ++i)
		{
			csShaders[i] = d3dUtil::CompileShader(compilePath, nullptr, kernelName[i], "cs_5_1");//Compile(compilePath, kernelName[i], nullptr, useCache);
			D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.pRootSignature = mRootSignature.Get();
			psoDesc.CS =
			{
				reinterpret_cast<BYTE*>(csShaders[i]->GetBufferPointer()),
				csShaders[i]->GetBufferSize()
			};
			psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso[i])));
		}
	}
}



void ComputeShader::BindRootSignature(ID3D12GraphicsCommandList* commandList) const
{
	commandList->SetComputeRootSignature(mRootSignature.Get());
}

void ComputeShader::BindRootSignature(ID3D12GraphicsCommandList* commandList, const DescriptorHeap* heap) const
{
	commandList->SetComputeRootSignature(mRootSignature.Get());
	heap->SetDescriptorHeap(commandList);
}

void ComputeShader::SetRes(ID3D12GraphicsCommandList* commandList, UINT id, const MObject* targetObj, UINT indexOffset, const std::type_info& tyid)const
{
#if defined(DEBUG) | defined(_DEBUG)
	if (targetObj == nullptr)
		throw "Resource Not Found!";
#else
	if (targetObj == nullptr) return;
#endif
	auto ite = mVariablesDict.Find(id);
#if defined(DEBUG) | defined(_DEBUG)
	if (!ite)
		throw "Resource Not Found!";
#else
	if (!ite) return;
#endif
	UINT rootSigPos = ite.Value();
	const ComputeShaderVariable& var = mVariablesVector[rootSigPos];


	switch (var.type)
	{
	case ComputeShaderVariable::Type::SRVDescriptorHeap:
	{
#if defined(DEBUG) | defined(_DEBUG)
		if (tyid != typeid(DescriptorHeap))
		{
			throw "Type Not Right!";
		}
#endif
		ID3D12DescriptorHeap* heap = ((const DescriptorHeap*)targetObj)->Get();
		commandList->SetComputeRootDescriptorTable(
			rootSigPos,
			((const DescriptorHeap*)targetObj)->hGPU(indexOffset)
		);
	}
	break;
	case ComputeShaderVariable::Type::UAVDescriptorHeap:
	{
#if defined(DEBUG) | defined(_DEBUG)
		if (tyid != typeid(DescriptorHeap))
		{
			throw "Type Not Right!";
		}
#endif
		ID3D12DescriptorHeap* heap = ((const DescriptorHeap*)targetObj)->Get();
		commandList->SetComputeRootDescriptorTable(
			rootSigPos,
			((const DescriptorHeap*)targetObj)->hGPU(indexOffset)
		);
	}
	break;
	case ComputeShaderVariable::Type::ConstantBuffer:
	{
#if defined(DEBUG) | defined(_DEBUG)
		if (tyid != typeid(UploadBuffer) && tyid != typeid(StructuredBuffer))
		{
			throw "Type Not Right!";
		}
#endif
		if (tyid == typeid(UploadBuffer))
		{
			const UploadBuffer* uploadBufferPtr = ((const UploadBuffer*)targetObj);
			commandList->SetComputeRootConstantBufferView(
				rootSigPos,
				uploadBufferPtr->GetAddress(indexOffset)
			);
		}
		else
		{
			const StructuredBuffer* sbufferPtr = ((const StructuredBuffer*)targetObj);
			commandList->SetComputeRootConstantBufferView(
				rootSigPos,
				sbufferPtr->GetAddress(0, indexOffset)
			);
		}
	}
	break;
	case ComputeShaderVariable::Type::StructuredBuffer:
	{
#if defined(DEBUG) | defined(_DEBUG)
		if (tyid != typeid(UploadBuffer) && tyid != typeid(StructuredBuffer))
		{
			throw "Type Not Right!";
		}
#endif
		if (tyid == typeid(UploadBuffer))
		{
			const UploadBuffer* uploadBufferPtr = ((const UploadBuffer*)targetObj);
			commandList->SetComputeRootShaderResourceView(
				rootSigPos,
				uploadBufferPtr->GetAddress(indexOffset)
			);
		}
		else
		{
			const StructuredBuffer* sbufferPtr = ((const StructuredBuffer*)targetObj);
			commandList->SetComputeRootShaderResourceView(
				rootSigPos,
				sbufferPtr->GetAddress(0, indexOffset)
			);
		}
	}
	break;
	case ComputeShaderVariable::Type::RWStructuredBuffer:
	{
#if defined(DEBUG) | defined(_DEBUG)
		if (tyid != typeid(StructuredBuffer))
		{
			throw "Type Not Right!";
		}
#endif
		StructuredBuffer* sbufferPtr = ((StructuredBuffer*)targetObj);
		commandList->SetComputeRootUnorderedAccessView(
			rootSigPos,
			sbufferPtr->GetAddress(0, indexOffset)
		);
	}
	break;
	}
}

void ComputeShader::SetStructuredBufferByAddress(ID3D12GraphicsCommandList* commandList, UINT id, D3D12_GPU_VIRTUAL_ADDRESS address) const
{
	auto ite = mVariablesDict.Find(id);
	if (!ite) return;
	UINT rootSigPos = ite.Value();
	const ComputeShaderVariable& var = mVariablesVector[rootSigPos];
	if (var.type == ComputeShaderVariable::Type::RWStructuredBuffer)
	{
		commandList->SetComputeRootUnorderedAccessView(
			rootSigPos,
			address);
	}
	else if (var.type == ComputeShaderVariable::Type::StructuredBuffer)
	{
		commandList->SetComputeRootShaderResourceView(
			rootSigPos,
			address
		);
	}
}

void ComputeShader::Dispatch(ID3D12GraphicsCommandList* commandList, UINT kernel, UINT x, UINT y, UINT z) const
{
	commandList->SetPipelineState(pso[kernel].Get());
	commandList->Dispatch(x, y, z);
}

void ComputeShader::DispatchIndirect(ID3D12GraphicsCommandList* commandList, UINT dispatchKernel, StructuredBuffer* indirectBuffer, UINT bufferElement, UINT bufferIndex) const
{
	commandList->SetPipelineState(pso[dispatchKernel].Get());
	commandList->ExecuteIndirect(cmdSig.GetSignature(), 1, indirectBuffer->GetResource(), indirectBuffer->GetAddressOffset(bufferElement, bufferIndex), nullptr, 0);
}