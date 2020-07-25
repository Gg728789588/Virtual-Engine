#include "Shader.h"
#include "../Singleton/ShaderID.h"
#include "Texture.h"
#include "../RenderComponent/DescriptorHeap.h"
#include "UploadBuffer.h"
#include <fstream>
#include "../JobSystem/JobInclude.h"
#include "Utility/ShaderIO.h"
#include "StructuredBuffer.h"
using Microsoft::WRL::ComPtr;
Shader::~Shader()
{

}

void Shader::BindRootSignature(ID3D12GraphicsCommandList* commandList)  const
{
	commandList->SetGraphicsRootSignature(mRootSignature.Get());
}

void Shader::BindRootSignature(ID3D12GraphicsCommandList* commandList, const DescriptorHeap* descHeap)  const
{
	commandList->SetGraphicsRootSignature(mRootSignature.Get());
	if (descHeap)
		descHeap->SetDescriptorHeap(commandList);
}

void Shader::GetPassPSODesc(UINT pass, D3D12_GRAPHICS_PIPELINE_STATE_DESC* targetPSO) const
{
	const Pass& p = allPasses[pass];
	if (p.vsShader) {
		targetPSO->VS =
		{
			reinterpret_cast<BYTE*>(p.vsShader->GetBufferPointer()),
			p.vsShader->GetBufferSize()
		};
	}
	if (p.psShader)
	{
		targetPSO->PS =
		{
			reinterpret_cast<BYTE*>(p.psShader->GetBufferPointer()),
			p.psShader->GetBufferSize()
		};
	}
	if (p.hsShader)
	{
		targetPSO->HS =
		{
			reinterpret_cast<BYTE*>(p.hsShader->GetBufferPointer()),
			p.hsShader->GetBufferSize()
		};
	}
	if (p.dsShader)
	{
		targetPSO->DS =
		{
			reinterpret_cast<BYTE*>(p.dsShader->GetBufferPointer()),
			p.dsShader->GetBufferSize()
		};
	}

	targetPSO->BlendState = p.blendState;
	targetPSO->RasterizerState = p.rasterizeState;
	targetPSO->pRootSignature = mRootSignature.Get();
	targetPSO->DepthStencilState = p.depthStencilState;
}
struct ShaderCompile
{
	uint passCount;
	vengine::vector<InputPass> inputPasses;
	vengine::vector<Pass>* passes;
	ShaderCompile(uint passCount, InputPass* ips, vengine::vector<Pass>* ps) :
		passCount(passCount), inputPasses(passCount), passes(ps)
	{
		for (uint i = 0; i < passCount; ++i)
		{
			inputPasses[i] = ips[i];
		}
	}
	void operator()()
	{
		for (int i = 0; i < passCount; ++i)
		{
			InputPass& ip = inputPasses[i];
			Pass& p = (*passes)[i];
			if (p.vsShader == nullptr)
				p.vsShader = d3dUtil::CompileShader(ip.filePath, nullptr, ip.vertex, "vs_5_1");//CompileShader(p.filePath, p.vertex, "vs_5_1", nullptr, useShaderCache);
			if (p.psShader == nullptr)
				p.psShader = d3dUtil::CompileShader(ip.filePath, nullptr, ip.fragment, "ps_5_1");// CompileShader(p.filePath, p.fragment, "ps_5_1", nullptr, useShaderCache);
		}
	}
};

Shader::Shader(ID3D12Device* device, const vengine::string& csoFilePath)
{
	ShaderIO::DecodeShader(csoFilePath, mVariablesVector, allPasses);
	mVariablesDict.Reserve(mVariablesVector.size() + 2);
	for (int i = 0; i < mVariablesVector.size(); ++i)
	{
		ShaderVariable& variable = mVariablesVector[i];
		mVariablesDict.Insert(ShaderID::PropertyToID(variable.name), i);
	}

	ArrayList<CD3DX12_ROOT_PARAMETER> allParameter;
	auto staticSamplers = d3dUtil::GetStaticSamplers();
	allParameter.reserve(VariableLength());
	ArrayList< CD3DX12_DESCRIPTOR_RANGE> allTexTable;
	allTexTable.reserve(VariableLength());
	IterateVariables([&](ShaderVariable& var) -> void {
		if (var.type == ShaderVariableType_SRVDescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
		else if (var.type == ShaderVariableType_UAVDescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
		});
	UINT offset = 0;
	IterateVariables([&](ShaderVariable& var) -> void
		{
			CD3DX12_ROOT_PARAMETER slotRootParameter;
			switch (var.type)
			{
			case ShaderVariableType_SRVDescriptorHeap:
				slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
				offset++;
				break;
			case ShaderVariableType_UAVDescriptorHeap:
				slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
				offset++;
				break;
			case ShaderVariableType_ConstantBuffer:
				slotRootParameter.InitAsConstantBufferView(var.registerPos, var.space);
				break;
			case ShaderVariableType_StructuredBuffer:
				slotRootParameter.InitAsShaderResourceView(var.registerPos, var.space);
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
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}
Shader::Shader(
	InputPass* passes, UINT passCount,
	ShaderVariable* shaderVariables, UINT shaderVarCount,
	ID3D12Device* device,
	JobBucket* compileJob
)
{
	//Create Pass
	allPasses.reserve(passCount);
	for (int i = 0; i < passCount; ++i)
	{
		InputPass& p = passes[i];
		Pass pp;
		pp.rasterizeState = p.rasterizeState;
		pp.depthStencilState = p.depthStencilState;
		pp.blendState = p.blendState;
		allPasses.push_back(pp);
	}

	if (compileJob)
	{
		compileJob->GetTask<ShaderCompile>(nullptr, 0, ShaderCompile(passCount, passes, &allPasses));
	}
	else {
		for (int i = 0; i < passCount; ++i)
		{
			InputPass& ip = passes[i];
			Pass& p = allPasses[i];
			if (p.vsShader == nullptr)
				p.vsShader = d3dUtil::CompileShader(ip.filePath, nullptr, ip.vertex, "vs_5_1");//CompileShader(p.filePath, p.vertex, "vs_5_1", nullptr, useShaderCache);
			if (p.psShader == nullptr)
				p.psShader = d3dUtil::CompileShader(ip.filePath, nullptr, ip.fragment, "ps_5_1");// CompileShader(p.filePath, p.fragment, "ps_5_1", nullptr, useShaderCache);
		}
	}

	mVariablesDict.Reserve(shaderVarCount + 2);
	mVariablesVector.reserve(shaderVarCount);
	for (int i = 0; i < shaderVarCount; ++i)
	{
		ShaderVariable& variable = shaderVariables[i];
		mVariablesDict.Insert(ShaderID::PropertyToID(variable.name), i);
		mVariablesVector.push_back(variable);

	}

	ArrayList<CD3DX12_ROOT_PARAMETER> allParameter;
	auto staticSamplers = d3dUtil::GetStaticSamplers();
	allParameter.reserve(VariableLength());
	ArrayList< CD3DX12_DESCRIPTOR_RANGE> allTexTable;
	allTexTable.reserve(VariableLength());
	IterateVariables([&](ShaderVariable& var) -> void {
		if (var.type == ShaderVariableType_SRVDescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
		else if (var.type == ShaderVariableType_UAVDescriptorHeap)
		{
			CD3DX12_DESCRIPTOR_RANGE texTable;
			texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, var.tableSize, var.registerPos, var.space);
			allTexTable.push_back(texTable);
		}
		});
	UINT offset = 0;
	IterateVariables([&](ShaderVariable& var) -> void
		{
			CD3DX12_ROOT_PARAMETER slotRootParameter;
			switch (var.type)
			{
			case ShaderVariableType_SRVDescriptorHeap:
				slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
				offset++;
				break;
			case ShaderVariableType_UAVDescriptorHeap:
				slotRootParameter.InitAsDescriptorTable(1, allTexTable.data() + offset);
				offset++;
				break;
			case ShaderVariableType_ConstantBuffer:
				slotRootParameter.InitAsConstantBufferView(var.registerPos, var.space);
				break;
			case ShaderVariableType_StructuredBuffer:
				slotRootParameter.InitAsShaderResourceView(var.registerPos, var.space);
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
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}


int Shader::GetPropertyRootSigPos(UINT id)  const
{
	auto ite = mVariablesDict.Find(id);
	if (!ite) return -1;
	return (int)ite.Value();
}

void Shader::SetRes(ID3D12GraphicsCommandList* commandList, UINT id, const MObject* targetObj, UINT indexOffset, const std::type_info& tyid)const
{
#if defined(DEBUG) || defined(_DEBUG)
	if (targetObj == nullptr) throw "Obj Null!";
#else
	if (targetObj == nullptr) return;
#endif
	auto ite = mVariablesDict.Find(id);
#if defined(DEBUG) || defined(_DEBUG)
	if (!ite) throw "Resource Not Found!";
#else
	if (!ite) return;
#endif
	UINT rootSigPos = ite.Value();
	const ShaderVariable& var = mVariablesVector[rootSigPos];
	const UploadBuffer* uploadBufferPtr;
	ID3D12DescriptorHeap* heap = nullptr;
	switch (var.type)
	{
	case ShaderVariableType_SRVDescriptorHeap:
#if defined(DEBUG) | defined(_DEBUG)
		if (tyid != typeid(DescriptorHeap))
		{
			throw "Type Not Right!";
		}
#endif
		heap = ((const DescriptorHeap*)targetObj)->Get();
		commandList->SetGraphicsRootDescriptorTable(
			rootSigPos,
			((const DescriptorHeap*)targetObj)->hGPU(indexOffset)
		);
		break;
	case ShaderVariableType_UAVDescriptorHeap:
#if defined(DEBUG) | defined(_DEBUG)
		if (tyid != typeid(DescriptorHeap))
		{
			throw "Type Not Right!";
		}
#endif
		heap = ((const DescriptorHeap*)targetObj)->Get();
		commandList->SetGraphicsRootDescriptorTable(
			rootSigPos,
			((const DescriptorHeap*)targetObj)->hGPU(indexOffset)
		);
		break;
	case ShaderVariableType_ConstantBuffer:
#if defined(DEBUG) | defined(_DEBUG)
		if (tyid != typeid(UploadBuffer) && tyid != typeid(StructuredBuffer))
		{
			throw "Type Not Right!";
		}
#endif
		if (tyid == typeid(UploadBuffer))
		{
			uploadBufferPtr = ((const UploadBuffer*)targetObj);
			commandList->SetGraphicsRootConstantBufferView(
				rootSigPos,
				uploadBufferPtr->GetAddress(indexOffset));
		}
		else
		{
			const StructuredBuffer* sbufferPtr = ((const StructuredBuffer*)targetObj);
			commandList->SetGraphicsRootConstantBufferView(
				rootSigPos,
				sbufferPtr->GetAddress(0, indexOffset));
		}
		break;
	case ShaderVariableType_StructuredBuffer:
#if defined(DEBUG) | defined(_DEBUG)
		if (tyid != typeid(UploadBuffer) && tyid != typeid(StructuredBuffer))
		{
			throw "Type Not Right!";
		}
#endif
		if (tyid == typeid(UploadBuffer))
		{
			uploadBufferPtr = ((const UploadBuffer*)targetObj);
			commandList->SetGraphicsRootShaderResourceView(
				rootSigPos,
				uploadBufferPtr->GetAddress(indexOffset));
		}
		else
		{
			const StructuredBuffer* sbufferPtr = ((const StructuredBuffer*)targetObj);
			commandList->SetGraphicsRootShaderResourceView(
				rootSigPos,
				sbufferPtr->GetAddress(0, indexOffset));
		}
		break;
	}
}

ShaderVariable Shader::GetVariable(vengine::string name)const
{
	auto ite = mVariablesDict.Find(ShaderID::PropertyToID(name));
	return mVariablesVector[ite.Value()];
}

ShaderVariable Shader::GetVariable(UINT id)const
{
	auto ite = mVariablesDict.Find(id);
	return mVariablesVector[ite.Value()];
}

bool Shader::TryGetShaderVariable(UINT id, ShaderVariable& targetVar)const
{
	auto ite = mVariablesDict.Find(id);
	if (!ite) return false;
	targetVar = mVariablesVector[ite.Value()];
	return true;
}

void Shader::SetStructuredBufferByAddress(ID3D12GraphicsCommandList* commandList, UINT id, D3D12_GPU_VIRTUAL_ADDRESS address)  const
{
	auto ite = mVariablesDict.Find(id);
	if (!ite) return;
	UINT rootSigPos = ite.Value();
	commandList->SetGraphicsRootShaderResourceView(
		rootSigPos,
		address);
}
