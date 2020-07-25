#include "ShaderCompiler.h"
#include "../JobSystem/JobInclude.h"
#include "ShaderUniforms.h"
#include "../Utility/StringUtility.h"
using namespace SCompile;
#define StructuredBuffer SCompile::StructuredBuffer_S
int ShaderCompiler::shaderIDCount = 0;

 HashMap<vengine::string, Shader*> ShaderCompiler::mShaders(53);
 HashMap<vengine::string, ComputeShader*> ShaderCompiler::mComputeShaders(53);

void LoadShader(const vengine::string& name, ID3D12Device* device, JobBucket* bucket, const vengine::string& path)
{
	Shader* sh = (Shader*)malloc(sizeof(Shader));
	if (bucket)
	{
		bucket->GetTask(nullptr, 0, [=]()->void
			{
				new (sh)Shader(device, path);
			});
	}
	else
		new (sh)Shader(device, path);
	ShaderCompiler::AddShader(name, sh);
}

void LoadComputeShader(const vengine::string& name, ID3D12Device* device, JobBucket* bucket, const vengine::string& path)
{
	ComputeShader* sh = (ComputeShader*)malloc(sizeof(ComputeShader));
	if (bucket)
	{
		bucket->GetTask(nullptr, 0, [=]()->void
			{
				new (sh)ComputeShader(path, device);
			});
	}
	else
		new (sh)ComputeShader(path, device);
	ShaderCompiler::AddComputeShader(name, sh);
}
void ShaderCompiler::AddShader(const vengine::string& str, Shader* shad)
{
	mShaders.Insert(str, shad);
}

void ShaderCompiler::AddComputeShader(const vengine::string& str, ComputeShader* shad)
{
	mComputeShaders.Insert(str, shad);
}

Shader const* ShaderCompiler::GetShader(const vengine::string& name)
{
	Shader* value = nullptr;
	mShaders.TryGet(name, value);
	return value;
}

ComputeShader const* ShaderCompiler::GetComputeShader(const vengine::string& name)
{
	ComputeShader* value = nullptr;
	mComputeShaders.TryGet(name, value);
	return value;
}

D3D12_RASTERIZER_DESC GetRasterize(D3D12_CULL_MODE cullMode, bool enableConservativeRaster = false)
{
	D3D12_RASTERIZER_DESC desc;
	desc.FillMode = D3D12_FILL_MODE_SOLID;
	desc.CullMode = cullMode;
	desc.FrontCounterClockwise = FALSE;
	desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	desc.DepthClipEnable = TRUE;
	desc.MultisampleEnable = FALSE;
	desc.AntialiasedLineEnable = FALSE;
	desc.ForcedSampleCount = 0;
	desc.ConservativeRaster = enableConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return desc;
}

D3D12_BLEND_DESC GetAlphaBlend()
{
	D3D12_BLEND_DESC blendDesc;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.AlphaToCoverageEnable = FALSE;


	D3D12_RENDER_TARGET_BLEND_DESC desc;
	desc.BlendEnable = TRUE;
	desc.LogicOpEnable = FALSE;
	desc.BlendOp = D3D12_BLEND_OP_ADD;
	desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	desc.SrcBlendAlpha = D3D12_BLEND_ZERO;
	desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	desc.DestBlendAlpha = D3D12_BLEND_ONE;
	desc.LogicOp = D3D12_LOGIC_OP_NOOP;
	desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE;

	for (uint i = 0; i < 8; ++i)
	{
		blendDesc.RenderTarget[i] = desc;
	}
	return blendDesc;
}
/*
void GetTonemapLut3DShader(ID3D12Device* device, JobBucket* bucket)
{
	const UINT KERNEL_COUNT = 1;
	vengine::string kernelNames[KERNEL_COUNT] =
	{
		"KGenLut3D_AcesTonemap"
	};
	const uint SHADER_VAR_COUNT = 3;
	ComputeShaderVariable vars[SHADER_VAR_COUNT] =
	{
		RWTexture3D::GetComputeVar(1, 0, 0, "_MainTex"),
		Texture2D::GetComputeVar(1, 0, 0, "_Curves"),
		ConstantBuffer::GetComputeVar(0, 0, "Params")
	};
	ComputeShader* cs = new ComputeShader("Shaders\\Lut3DBaker.compute", kernelNames, KERNEL_COUNT, vars, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddComputeShader("Lut3DBaker", cs);
}

void GetHIZMipGeneratorShader(ID3D12Device* device, JobBucket* bucket)
{
	const uint KERNEL_COUNT = 3;
	vengine::string kernelNames[KERNEL_COUNT] =
	{
		"GenerateMip",
		"GenerateMip1",
		"UpdateDepth"
	};
	const uint SHADER_VAR_COUNT = 2;
	ComputeShaderVariable vars[SHADER_VAR_COUNT] =
	{
		RWTexture2D::GetComputeVar(9, 0, 0, "_DepthTexture"),
		Texture2D::GetComputeVar(1, 0, 0, "_CameraDepthTexture")
	};

	ComputeShader* cs = new ComputeShader("Shaders/HizGenerator.compute", kernelNames, KERNEL_COUNT, vars, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddComputeShader("HIZGenerator", cs);
}

void GetMipGeneratorShader(ID3D12Device* device, JobBucket* bucket)
{
	const uint KERNEL_COUNT = 1;
	vengine::string kernelNames[KERNEL_COUNT] =
	{
		"GenerateMip"
	};
	const uint SHADER_VAR_COUNT = 1;
	ComputeShaderVariable vars[SHADER_VAR_COUNT] =
	{
		RWTexture2D::GetComputeVar(9, 0, 0, "_DepthTexture"),
	};
	ComputeShader* cs = new ComputeShader("Shaders/MipGenerator.compute", kernelNames, KERNEL_COUNT, vars, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddComputeShader("MipGenerator", cs);
}

void GetAutoExposure(ID3D12Device* device, JobBucket* bucket)
{
	const uint KERNEL_COUNT = 1;
	vengine::string kernelNames[KERNEL_COUNT] =
	{
		"MAIN"
	};
	const uint SHADER_VAR_COUNT = 4;
	ComputeShaderVariable vars[SHADER_VAR_COUNT] =
	{
		StructuredBuffer::GetComputeVar(0, 0, "_HistogramBuffer"),
		Texture2D::GetComputeVar(1,1,0, "_Source"),
		RWTexture2D::GetComputeVar(2,0,0,"_Destination"),
		ConstantBuffer::GetComputeVar(0, 0, "Params")
	};
	ComputeShader* cs = new ComputeShader("Shaders/AutoExposure.compute", kernelNames, KERNEL_COUNT, vars, SHADER_VAR_COUNT, device, bucket);
	ShaderCompiler::AddComputeShader("AutoExposure", cs);
}*/

void CompileShaders(ID3D12Device* device, JobBucket* bucket, const vengine::string& path)
{
	std::ifstream ifs(path.c_str());
	if (!ifs) return;
	vengine::vector<vengine::string> commandLines;
	StringUtil::ReadLines(ifs, commandLines);
	vengine::vector<vengine::string> separateCommands;
	for (auto i = commandLines.begin(); i != commandLines.end(); ++i)
	{
		StringUtil::Split(*i, ' ', separateCommands);
		if (separateCommands.size() != 3) continue;
		if (separateCommands[0] == "compute")
		{
			LoadComputeShader(separateCommands[1], device, bucket, separateCommands[2]);
		}
		else if (separateCommands[0] == "shader")
		{
			LoadShader(separateCommands[1], device, bucket, separateCommands[2]);
		}
	}
}

void ShaderCompiler::Init(ID3D12Device* device, JobSystem* jobSys)
{
#ifdef NDEBUG
#define SHADER_MULTICORE_COMPILE	
#endif
	JobBucket* bucket = nullptr;
#ifdef SHADER_MULTICORE_COMPILE
	bucket = jobSys->GetJobBucket();
#endif
	CompileShaders(device, bucket, "Data/ShaderCompileList.inf");
	/*GetTonemapLut3DShader(device, bucket);
	GetHIZMipGeneratorShader(device, bucket);
	GetMipGeneratorShader(device, bucket);
	GetAutoExposure(device, bucket);*/
#ifdef SHADER_MULTICORE_COMPILE
	jobSys->ExecuteBucket(bucket, 1);
#endif
}

void ShaderCompiler::Dispose()
{
	mComputeShaders.Clear();
	mShaders.Clear();
}
#undef StructuredBuffer