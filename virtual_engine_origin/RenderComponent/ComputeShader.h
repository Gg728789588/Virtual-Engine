#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MetaLib.h"
#include "Shader.h"
#include "../Common/HashMap.h"
#include "Utility/CommandSignature.h"
class JobBucket;
struct ComputeShaderVariable
{
	enum Type
	{
		ConstantBuffer, StructuredBuffer, RWStructuredBuffer, SRVDescriptorHeap, UAVDescriptorHeap
	};
	vengine::string name;
	Type type;
	UINT tableSize;
	UINT registerPos;
	UINT space;
	ComputeShaderVariable() {}
	ComputeShaderVariable(
		const vengine::string& name,
		Type type,
		UINT tableSize,
		UINT registerPos,
		UINT space
	) : name(name),
		type(type),
		tableSize(tableSize),
		registerPos(registerPos),
		space(space)
	{}
};

class DescriptorHeap;
class StructuredBuffer;
class ComputeShaderCompiler;
class ComputeShaderReader;
class ComputeShader
{
	friend class ComputeShaderCompiler;
	friend class ComputeShaderReader;
private:
	vengine::vector<Pass> allPasses;
	
	HashMap<UINT, UINT> mVariablesDict;
	vengine::vector<ComputeShaderVariable> mVariablesVector;
	vengine::vector<Microsoft::WRL::ComPtr<ID3DBlob>> csShaders;
	vengine::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pso;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	CommandSignature cmdSig;
	void SetRes(ID3D12GraphicsCommandList* commandList, UINT id, const MObject* targetObj, UINT indexOffset, const std::type_info& tyid) const;
public:
	ComputeShader(
		const vengine::string& csoFilePath,
		ID3D12Device* device);

	ComputeShader(
		const vengine::string& compilePath,
		vengine::string* kernelName, UINT kernelCount,
		ComputeShaderVariable* allShaderVariables, UINT varSize,
		ID3D12Device* device,
		JobBucket* compileJob);
	size_t VariableLength() const { return mVariablesVector.size(); }
	void BindRootSignature(ID3D12GraphicsCommandList* commandList) const;
	void BindRootSignature(ID3D12GraphicsCommandList* commandList, const DescriptorHeap* heap) const;
	void SetStructuredBufferByAddress(ID3D12GraphicsCommandList* commandList, UINT id, D3D12_GPU_VIRTUAL_ADDRESS address) const;
	void Dispatch(ID3D12GraphicsCommandList* cList, UINT kernel, UINT x, UINT y, UINT z) const;
	void DispatchIndirect(ID3D12GraphicsCommandList* cList, UINT dispatchKernel, StructuredBuffer* indirectBuffer, UINT bufferElement, UINT bufferIndex) const;
	template<typename Func>
	void IterateVariables(const Func& f)
	{
		for (int i = 0; i < mVariablesVector.size(); ++i)
		{
			f(mVariablesVector[i]);
		}
	}
	template <typename T>
	void SetResource(ID3D12GraphicsCommandList* commandList, UINT id, T* targetObj, UINT indexOffset) const
	{
		SetRes(commandList, id, targetObj, indexOffset, typeid(PureType_t<T>));
	}
};