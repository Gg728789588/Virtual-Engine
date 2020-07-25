#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/HashMap.h"
class JobSystem;
class Shader;
class ComputeShader;
class ID3D12Device;
class MaterialManager;
class ShaderCompiler
{
private:
	static int shaderIDCount;
	static HashMap<vengine::string, ComputeShader*> mComputeShaders;
	static HashMap<vengine::string, Shader*> mShaders;
public:
	static Shader const* GetShader(const vengine::string& name);
	static ComputeShader const* GetComputeShader(const vengine::string& name);
	static void AddShader(const vengine::string& str, Shader* shad);
	static void AddComputeShader(const vengine::string& str, ComputeShader* shad);
	static void Init(ID3D12Device* device, JobSystem* jobSys);
	static void Dispose();
};