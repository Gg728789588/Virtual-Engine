#pragma once
#include "../Common/d3dUtil.h"

#include "../Common/MObject.h"
#include "../Common/MetaLib.h"
#include "../Common/HashMap.h"
class JobBucket;
class DescriptorHeap;
struct Pass
{
	Microsoft::WRL::ComPtr<ID3DBlob> vsShader = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> psShader = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> hsShader = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> dsShader = nullptr;
	D3D12_RASTERIZER_DESC rasterizeState;
	D3D12_DEPTH_STENCIL_DESC depthStencilState;
	D3D12_BLEND_DESC blendState;
};

struct InputPass
{
	vengine::string filePath;
	vengine::string vertex;
	vengine::string fragment;
	D3D12_RASTERIZER_DESC rasterizeState;
	D3D12_DEPTH_STENCIL_DESC depthStencilState;
	D3D12_BLEND_DESC blendState;
};

enum ShaderVariableType
{
	ShaderVariableType_ConstantBuffer,
	ShaderVariableType_SRVDescriptorHeap, 
	ShaderVariableType_UAVDescriptorHeap, 
	ShaderVariableType_StructuredBuffer
};
struct ShaderVariable
{
	vengine::string name;
	ShaderVariableType type;
	UINT tableSize;
	UINT registerPos;
	UINT space;
	ShaderVariable() {}
	ShaderVariable(
		const vengine::string& name,
		ShaderVariableType type,
		UINT tableSize,
		UINT registerPos,
		UINT space
	) : name(name),
		type(type),
		tableSize(tableSize),
		registerPos(registerPos),
		space(space) {}
};
class Shader
{
private:
	vengine::vector<Pass> allPasses;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	HashMap<UINT, UINT> mVariablesDict;
	vengine::vector<ShaderVariable> mVariablesVector;
	void SetRes(ID3D12GraphicsCommandList* commandList, UINT id, const MObject* targetObj, UINT indexOffset, const std::type_info& tyid)const;
public:
	Shader() {}
	~Shader();
	Shader(
		InputPass* passes, UINT passCount,
		ShaderVariable* shaderVariables, UINT shaderVarCount,
		ID3D12Device* device,
		JobBucket* compileJob
	);
	Shader(ID3D12Device* device, const vengine::string& csoFilePath);
	void GetPassPSODesc(UINT pass, D3D12_GRAPHICS_PIPELINE_STATE_DESC* targetPSO) const;
	ShaderVariable GetVariable(vengine::string name)const;
	ShaderVariable GetVariable(UINT id)const;
	void BindRootSignature(ID3D12GraphicsCommandList* commandList, const DescriptorHeap* descHeap)  const;
	void BindRootSignature(ID3D12GraphicsCommandList* commandList)  const;
	int GetPropertyRootSigPos(UINT id)  const;
	ID3D12RootSignature* GetSignature() const { return mRootSignature.Get(); }
	
	void SetStructuredBufferByAddress(ID3D12GraphicsCommandList* commandList, UINT id, D3D12_GPU_VIRTUAL_ADDRESS address)  const;
	bool TryGetShaderVariable(UINT id, ShaderVariable& targetVar)const;
	size_t VariableLength() const { return mVariablesVector.size(); }
	template<typename Func>
	void IterateVariables(Func&& f)
	{
		for (int i = 0; i < mVariablesVector.size(); ++i)
		{
			f(mVariablesVector[i]);
		}
	}
	template <typename T>
	void SetResource(ID3D12GraphicsCommandList* commandList, UINT id, T* targetObj, UINT indexOffset)const
	{
		SetRes(commandList, id, targetObj, indexOffset, typeid(PureType_t<T>));
	}
};

