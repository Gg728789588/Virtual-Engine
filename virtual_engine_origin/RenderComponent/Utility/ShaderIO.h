#pragma once
#include "../../Common/d3dUtil.h"
#include "../Shader.h"
#include "../ComputeShader.h"
class ShaderIO
{
public:
	static void DecodeShader(
		const vengine::string& fileName,
		vengine::vector<ShaderVariable>& vars,
		vengine::vector<Pass>& passes);
	static void DecodeComputeShader(
		const vengine::string& fileName,
		vengine::vector<ComputeShaderVariable>& vars,
		vengine::vector<Microsoft::WRL::ComPtr<ID3DBlob>>& datas);
};