#pragma once
#include <d3d12.h>
#include "Common/vector.h"
#include <memory>
#include "Common/MObject.h"
#include <fstream>
#include <d3d12.h>
#include "d3dx12.h"
#include "Common/MObject.h"
#include "Common/vstring.h"
#include "Common/HashMap.h"
namespace SCompile {
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
	struct Pass
	{
		vengine::string vsShaderr;
		vengine::string psShader;
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
	struct KernelDescriptor
	{
		vengine::string name;
		ObjectPtr<vengine::vector<vengine::string>> macros;
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
	struct PassDescriptor
	{
		vengine::string name;
		vengine::string vertex;
		vengine::string fragment;
		vengine::string hull;
		vengine::string domain;
		ObjectPtr<vengine::vector<vengine::string>> macros;
		D3D12_RASTERIZER_DESC rasterizeState;
		D3D12_DEPTH_STENCIL_DESC depthStencilState;
		D3D12_BLEND_DESC blendState;
	};
	struct ShaderUniform
	{
		static HashMap<vengine::string, uint32_t> uMap;
		//Change Here to Add New Command
		static void Init();
		static void GetShaderRootSigData(const vengine::string& path, vengine::vector<ShaderVariable>& vars, vengine::vector<PassDescriptor>& GetPasses);
		static void GetComputeShaderRootSigData(const vengine::string& path, vengine::vector<ComputeShaderVariable>& vars, vengine::vector<KernelDescriptor>& GetPasses);
	};
}