#include "ShaderIO.h"
#include <fstream>


template <typename T>
void DragData(std::ifstream& ifs, T& data)
{
	ifs.read((char*)&data, sizeof(T));
}
template <>
void DragData<vengine::string>(std::ifstream& ifs, vengine::string& str)
{
	uint32_t length = 0;
	DragData<uint32_t>(ifs, length);
	str.clear();
	str.resize(length);
	ifs.read(str.data(), length);
}
void ShaderIO::DecodeShader(
	const vengine::string& fileName,
	vengine::vector<ShaderVariable>& vars,
	vengine::vector<Pass>& passes)
{
	vars.clear();
	passes.clear();
	uint varSize = 0;
	std::ifstream ifs(fileName.data(), std::ios::binary);
	if (!ifs) return;
	DragData<uint>(ifs, varSize);
	vars.resize(varSize);
	for (auto i = vars.begin(); i != vars.end(); ++i)
	{
		DragData<vengine::string>(ifs, i->name);
		DragData<ShaderVariableType>(ifs, i->type);
		DragData<uint>(ifs, i->tableSize);
		DragData<uint>(ifs, i->registerPos);
		DragData<uint>(ifs, i->space);
	}
	uint functionCount = 0;
	DragData<uint>(ifs, functionCount);
	vengine::vector<Microsoft::WRL::ComPtr<ID3DBlob>> functions(functionCount);
	for (uint i = 0; i < functionCount; ++i)
	{
		uint64_t codeSize = 0;
		DragData(ifs, codeSize);
		if(codeSize > 0)
		{
			D3DCreateBlob(codeSize, &functions[i]);
			ifs.read((char*)functions[i]->GetBufferPointer(), codeSize);
		}
	}
	uint passSize = 0;
	DragData<uint>(ifs, passSize);
	passes.resize(passSize);
	for (auto i = passes.begin(); i != passes.end(); ++i)
	{
		DragData(ifs, i->rasterizeState);
		DragData(ifs, i->depthStencilState);
		DragData(ifs, i->blendState);
		int vertIndex = 0, fragIndex = 0, hullIndex = 0, domainIndex = 0;
		DragData(ifs, vertIndex);
		DragData(ifs, hullIndex);
		DragData(ifs, domainIndex);
		DragData(ifs, fragIndex);
		i->vsShader = vertIndex >= 0 ? functions[vertIndex] : nullptr;
		i->hsShader = hullIndex >= 0 ? functions[hullIndex] : nullptr;
		i->dsShader = domainIndex >= 0 ? functions[domainIndex] : nullptr;
		i->psShader = fragIndex >= 0 ? functions[fragIndex] : nullptr;
	}
}

void ShaderIO::DecodeComputeShader(
	const vengine::string& fileName,
	vengine::vector<ComputeShaderVariable>& vars,
	vengine::vector<Microsoft::WRL::ComPtr<ID3DBlob>>& datas)
{
	vars.clear();
	datas.clear();
	uint varSize = 0;
	std::ifstream ifs(fileName.data(), std::ios::binary);
	if (!ifs) return;
	DragData<uint>(ifs, varSize);
	vars.resize(varSize);
	for (auto i = vars.begin(); i != vars.end(); ++i)
	{
		DragData<vengine::string>(ifs, i->name);
		DragData<ComputeShaderVariable::Type>(ifs, i->type);
		DragData<uint>(ifs, i->tableSize);
		DragData<uint>(ifs, i->registerPos);
		DragData<uint>(ifs, i->space);
	}
	uint kernelSize = 0;
	DragData<uint>(ifs, kernelSize);
	datas.resize(kernelSize);
	for (auto i = datas.begin(); i != datas.end(); ++i)
	{
		uint64_t kernelSize = 0;
		DragData<uint64_t>(ifs, kernelSize);
		D3DCreateBlob(kernelSize, &*i);
		ifs.read((char*)(*i)->GetBufferPointer(), kernelSize);
	}
}
