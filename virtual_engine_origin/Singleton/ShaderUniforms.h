#pragma once
#include "../RenderComponent/Shader.h"
#include "../RenderComponent/ComputeShader.h"

namespace SCompile {
	struct TextureCube
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_SRVDescriptorHeap, tableSize, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::SRVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct Texture2D
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_SRVDescriptorHeap, tableSize, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::SRVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct Texture3D
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_SRVDescriptorHeap, tableSize, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::SRVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct Texture2DArray
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_SRVDescriptorHeap, tableSize, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::SRVDescriptorHeap, tableSize, regis, space);
		}
	};
	struct StructuredBuffer_S
	{
		inline static ShaderVariable GetShaderVar(UINT regis, UINT space, const vengine::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_StructuredBuffer, 0, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT regis, UINT space, const vengine::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::StructuredBuffer, 0, regis, space);
		}
	};

	struct ConstantBuffer
	{
		inline static ShaderVariable GetShaderVar(UINT regis, UINT space, const vengine::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_ConstantBuffer, 0, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT regis, UINT space, const vengine::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::ConstantBuffer, 0, regis, space);
		}
	};


	struct RWTexture2D
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_UAVDescriptorHeap, tableSize, regis, space);
		}
		inline static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::UAVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct RWTexture3D
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_UAVDescriptorHeap, tableSize, regis, space);
		}
		inline	static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::UAVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct RWTexture2DArray
	{
		inline static ShaderVariable GetShaderVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ShaderVariable(name, ShaderVariableType_UAVDescriptorHeap, tableSize, regis, space);
		}
		inline	static ComputeShaderVariable GetComputeVar(UINT tableSize, UINT regis, UINT space, const vengine::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::UAVDescriptorHeap, tableSize, regis, space);
		}
	};

	struct RWStructuredBuffer
	{
		inline static ComputeShaderVariable GetComputeVar(UINT regis, UINT space, const vengine::string& name)
		{
			return ComputeShaderVariable(name, ComputeShaderVariable::RWStructuredBuffer, 0, regis, space);
		}
	};
	struct PassDescriptor
	{
		vengine::string name;
		vengine::string vertex;
		vengine::string fragment;
		D3D12_RASTERIZER_DESC rasterizeState;
		D3D12_DEPTH_STENCIL_DESC depthStencilState;
		D3D12_BLEND_DESC blendState;
	};
	void GetShaderRootSigData(const vengine::string& path, vengine::vector<ShaderVariable>& vars, vengine::vector<PassDescriptor>& GetPasses);
	void GetComputeShaderRootSigData(const vengine::string& path, vengine::vector<ComputeShaderVariable>& vars, vengine::vector<vengine::string>& GetPasses);
}