// ShaderCompiler.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#define _CRT_SECURE_NO_WARNINGS
#include "ICompileIterator.h"
#include "DirectCompile.h"
#include "StringUtility.h"
#include  <stdio.h>
#include  <io.h>
#include "Common/vector.h"
#include "BatchCompile.h"
#include <fstream>
#include "ShaderUniforms.h"
#include "JobSystem/JobInclude.h"
#include <atomic>
#include <windows.h> 
#include <tchar.h>
#include <strsafe.h>
#include <string>
#include "Common/HashMap.h"
#include "Common/vstring.h"

using namespace std;
using namespace SCompile;

static bool g_needCommandOutput = true;
static const HANDLE g_hChildStd_IN_Rd = NULL;
static const HANDLE g_hChildStd_IN_Wr = NULL;
static const HANDLE g_hChildStd_OUT_Rd = NULL;
static const HANDLE g_hChildStd_OUT_Wr = NULL;
struct ProcessorData
{
	_PROCESS_INFORMATION piProcInfo;
	bool bSuccess;
};

void CreateChildProcess(const vengine::string& cmd, ProcessorData* data)
{
	if (g_needCommandOutput)
	{
		cout << cmd << endl;
		system(cmd.c_str());
		memset(data, 0, sizeof(ProcessorData));
		return;
	}

	PROCESS_INFORMATION piProcInfo;

	static HANDLE g_hInputFile = NULL;
	std::wstring ws;
	ws.resize(cmd.length());
	for (uint i = 0; i < cmd.length(); ++i)
	{
		ws[i] = cmd[i];
	}

	//PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	// Set up members of the PROCESS_INFORMATION structure. 

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 

	bSuccess = CreateProcess(NULL,
		ws.data(),     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 
	data->bSuccess = bSuccess;
	data->piProcInfo = piProcInfo;
}

void WaitChildProcess(ProcessorData* data)
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{

	// If an error occurs, exit the application. 
	if (data->bSuccess)
	{
		auto&& piProcInfo = data->piProcInfo;
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 
		WaitForSingleObject(piProcInfo.hProcess, INFINITE);
		WaitForSingleObject(piProcInfo.hThread, INFINITE);
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
		// Close handles to the stdin and stdout pipes no longer needed by the child process.
		// If they are not explicitly closed, there is no way to recognize that the child process has ended.

		CloseHandle(g_hChildStd_OUT_Wr);
		CloseHandle(g_hChildStd_IN_Rd);
	}
}


vengine::string start;
vengine::string shaderTypeCmd;
vengine::string fullOptimize;
vengine::string funcName;
vengine::string output;
vengine::string macro_compile;
vengine::string dxcversion;
vengine::string dxcpath;
vengine::string fxcversion;
vengine::string fxcpath;
enum class Compiler : bool
{
	DXC = false,
	FXC = true
};
Compiler compilerUsage;
void InitRegisteData()
{
	ifstream ifs("DXC\\register.txt");
	if (!ifs)
	{
		cout << "Register.txt not found in DXC folder!" << endl;
		system("pause");
		exit(0);
	}
	vengine::vector<vengine::string> lines;
	ReadLines(ifs, lines);
	vengine::vector<vengine::string> splitResult;
	for (auto ite = lines.begin(); ite != lines.end(); ++ite)
	{
		RemoveCharFromString(*ite, ' ');
		Split(*ite, ':', splitResult);
		if (splitResult.size() == 2)
		{
			ToUpper(splitResult[0]);
			if (splitResult[0] == "DXCSM")
			{
				dxcversion = splitResult[1];
			}
			else if (splitResult[0] == "DXCPATH")
			{
				dxcpath = "DXC\\" + splitResult[1];
			}
			else if (splitResult[0] == "FXCSM")
			{
				fxcversion = splitResult[1];
			}
			else if (splitResult[0] == "FXCPATH")
			{
				fxcpath = "DXC\\" + splitResult[1];
			}
			else if (splitResult[0] == "COMPILERUSAGE")
			{
				ToUpper(splitResult[1]);
				if (splitResult[1] == "DXC")
				{
					compilerUsage = Compiler::DXC;
				}
				else
				{
					compilerUsage = Compiler::FXC;
				}
			}
		}
	}
}
struct CompileFunctionCommand
{
	vengine::string name;
	ShaderType type;
	ObjectPtr<vengine::vector<vengine::string>> macros;
};
void GenerateDXCCommand(
	const vengine::string& fileName,
	const vengine::string& functionName,
	const vengine::string& resultFileName,
	const ObjectPtr<vengine::vector<vengine::string>>& macros,
	ShaderType shaderType,
	vengine::string& cmdResult
) {
	vengine::string shaderTypeName;
	switch (shaderType)
	{
	case ShaderType::ComputeShader:
		shaderTypeName = "cs_";
		break;
	case ShaderType::VertexShader:
		shaderTypeName = "vs_";
		break;
	case ShaderType::HullShader:
		shaderTypeName = "hs_";
		break;
	case ShaderType::DomainShader:
		shaderTypeName = "ds_";
		break;
	case ShaderType::GeometryShader:
		shaderTypeName = "gs_";
		break;
	case ShaderType::PixelShader:
		shaderTypeName = "ps_";
		break;
	default:
		shaderTypeName = " ";
		break;
	}
	shaderTypeName += dxcversion;
	cmdResult.clear();
	cmdResult.reserve(50);
	cmdResult += dxcpath + start + shaderTypeCmd + shaderTypeName + fullOptimize;
	if (macros && !macros->empty())
	{
		for (auto ite = macros->begin(); ite != macros->end(); ++ite)
		{
			cmdResult += macro_compile;
			cmdResult += *ite + "=1 ";
		}
	}
	cmdResult += funcName + functionName + output + '\"' + resultFileName + '\"' + " " + '\"' + fileName + '\"';
}
void GenerateFXCCommand(
	const vengine::string& fileName,
	const vengine::string& functionName,
	const vengine::string& resultFileName,
	const ObjectPtr<vengine::vector<vengine::string>>& macros,
	ShaderType shaderType,
	vengine::string& cmdResult
) {
	vengine::string shaderTypeName;
	switch (shaderType)
	{
	case ShaderType::ComputeShader:
		shaderTypeName = "cs_";
		break;
	case ShaderType::VertexShader:
		shaderTypeName = "vs_";
		break;
	case ShaderType::HullShader:
		shaderTypeName = "hs_";
		break;
	case ShaderType::DomainShader:
		shaderTypeName = "ds_";
		break;
	case ShaderType::GeometryShader:
		shaderTypeName = "gs_";
		break;
	case ShaderType::PixelShader:
		shaderTypeName = "ps_";
		break;
	default:
		shaderTypeName = " ";
		break;
	}
	shaderTypeName += fxcversion;
	cmdResult.clear();
	cmdResult.reserve(50);
	cmdResult += fxcpath + start + shaderTypeCmd + shaderTypeName + fullOptimize;
	if (macros && !macros->empty())
	{

		for (auto ite = macros->begin(); ite != macros->end(); ++ite)
		{
			cmdResult += macro_compile;
			cmdResult += *ite + "=1 ";
		}
	}
	cmdResult += funcName + functionName + output + '\"' + resultFileName + '\"' + " " + '\"' + fileName + '\"';
}

template <typename T>
void PutIn(vengine::string& c, const T& data)
{
	T* cc = &((T&)data);
	size_t siz = c.size();
	c.resize(siz + sizeof(T));
	memcpy(c.data() + siz, cc, sizeof(T));
}
template <>
void PutIn<vengine::string>(vengine::string& c, const vengine::string& data)
{
	PutIn<uint>(c, (uint)data.length());
	size_t siz = c.size();
	c.resize(siz + data.length());
	memcpy(c.data() + siz, data.data(), data.length());
}
template <typename T>
void DragData(ifstream& ifs, T& data)
{
	ifs.read((char*)&data, sizeof(T));
}
template <>
void DragData<vengine::string>(ifstream& ifs, vengine::string& str)
{
	uint32_t length = 0;
	DragData<uint32_t>(ifs, length);
	str.clear();
	str.resize(length);
	ifs.read(str.data(), length);
}
struct PassFunction
{
	vengine::string name;
	ObjectPtr<vengine::vector<vengine::string>> macros;
	PassFunction(const vengine::string& name,
		const ObjectPtr<vengine::vector<vengine::string>>& macros) :
		name(name),
		macros(macros) {}
	PassFunction() {}
	bool operator==(const PassFunction& p) const noexcept
	{
		bool cur = macros.operator bool();
		bool pCur = p.macros.operator bool();
		if (name == p.name)
		{
			if ((!pCur && !cur))
			{
				return true;
			}
			else if (cur && pCur && macros->size() == p.macros->size())
			{
				for (uint i = 0; i < macros->size(); ++i)
					if ((*macros)[i] != (*p.macros)[i]) return false;
				return true;
			}
		}
		return false;
	}
	bool operator!=(const PassFunction& p) const noexcept
	{
		return !operator==(p);
	}
};
struct PassFunctionHash
{
	size_t operator()(const PassFunction& pf) const noexcept
	{
		hash<vengine::string> hashStr;
		size_t h = hashStr(pf.name);
		if (pf.macros)
		{
			for (uint i = 0; i < pf.macros->size(); ++i)
			{
				h <<= 4;
				h ^= hashStr((*pf.macros)[i]);
			}
		}
		return h;
	}
};
void CompileShader(
	const vengine::string& fileName,
	const vengine::string& propertyPath,
	const vengine::string& tempFilePath,
	vengine::string& resultData)
{
	vengine::vector<ShaderVariable> vars;
	vengine::vector<PassDescriptor> passDescs;
	ShaderUniform::GetShaderRootSigData(propertyPath, vars, passDescs);

	resultData.clear();
	resultData.reserve(32768);
	PutIn<uint>(resultData, (uint)vars.size());
	for (auto i = vars.begin(); i != vars.end(); ++i)
	{
		PutIn<vengine::string>(resultData, i->name);
		PutIn<ShaderVariableType>(resultData, i->type);
		PutIn<uint>(resultData, i->tableSize);
		PutIn<uint>(resultData, i->registerPos);
		PutIn<uint>(resultData, i->space);
	}

	auto func = [&](ProcessorData* pData, vengine::string const& str)->void
	{
		//TODO
		uint64_t fileSize;
		WaitChildProcess(pData);
		//CreateChildProcess(,);
		//system(command.c_str());
		fileSize = 0;
		ifstream ifs(str.data(), ios::binary);
		if (!ifs) return;
		ifs.seekg(0, ios::end);
		fileSize = ifs.tellg();
		ifs.seekg(0, ios::beg);
		PutIn<uint64_t>(resultData, fileSize);
		if (fileSize == 0) return;
		size_t originSize = resultData.size();
		resultData.resize(fileSize + originSize);
		ifs.read(resultData.data() + originSize, fileSize);
	};
	HashMap<PassFunction, std::pair<ShaderType, uint>, PassFunctionHash> passMap(passDescs.size() * 2);
	for (auto i = passDescs.begin(); i != passDescs.end(); ++i)
	{
		auto findFunc = [&](const vengine::string& namestr, const ObjectPtr<vengine::vector<vengine::string>>& macros, ShaderType type)->void
		{
			PassFunction name(namestr, macros);
			if (name.name.empty()) return;
			if (!passMap.Contains(name))
			{
				passMap.Insert(name, std::pair<ShaderType, uint>(type, (uint)passMap.Size()));
			}
		};
		findFunc(i->vertex, i->macros, ShaderType::VertexShader);
		findFunc(i->hull, i->macros, ShaderType::HullShader);
		findFunc(i->domain, i->macros, ShaderType::DomainShader);
		findFunc(i->fragment, i->macros, ShaderType::PixelShader);
	}
	vengine::vector<CompileFunctionCommand> functionNames(passMap.Size());
	PutIn<uint>(resultData, (uint)passMap.Size());
	passMap.IterateAll([&](uint i, PassFunction const& key, std::pair<ShaderType, uint>& value)->void
		{
			CompileFunctionCommand cmd;
			cmd.macros = key.macros;
			cmd.name = key.name;
			cmd.type = value.first;
			functionNames[value.second] = cmd;
		});
	vengine::string commandCache;
	ArrayList<ProcessorData> datas(functionNames.size());
	vengine::vector<vengine::string> strs(functionNames.size());
	for (uint i = 0; i < functionNames.size(); ++i)
	{
		strs[i] = vengine::to_string(i) + tempFilePath;
		if (compilerUsage == Compiler::FXC)
		{
			GenerateFXCCommand(
				fileName, functionNames[i].name, strs[i],
				functionNames[i].macros, functionNames[i].type, commandCache);
		}
		else
		{
			GenerateDXCCommand(
				fileName, functionNames[i].name, strs[i],
				functionNames[i].macros, functionNames[i].type, commandCache);
		}
		CreateChildProcess(commandCache, &datas[i]);

	}

	for (uint i = 0; i < functionNames.size(); ++i)
	{
		func(&datas[i], strs[i]);
	}

	PutIn<uint>(resultData, (uint)passDescs.size());
	for (auto i = passDescs.begin(); i != passDescs.end(); ++i)
	{
		PutIn(resultData, i->rasterizeState);
		PutIn(resultData, i->depthStencilState);
		PutIn(resultData, i->blendState);
		auto PutInFunc = [&](const vengine::string& value, const ObjectPtr<vengine::vector<vengine::string>>& macros)->void
		{
			PassFunction psf(value, macros);
			if (value.empty() || !passMap.Contains(psf))
				PutIn<int>(resultData, -1);
			else
				PutIn<int>(resultData, (int)passMap[psf].second);
		};
		PutInFunc(i->vertex, i->macros);
		PutInFunc(i->hull, i->macros);
		PutInFunc(i->domain, i->macros);
		PutInFunc(i->fragment, i->macros);
	}
	for (auto i = strs.begin(); i != strs.end(); ++i)
		remove(i->c_str());
}

void CompileComputeShader(
	const vengine::string& fileName,
	const vengine::string& propertyPath,
	const vengine::string& tempFilePath,
	vengine::string& resultData)
{
	vengine::vector<ComputeShaderVariable> vars;
	vengine::vector<KernelDescriptor> passDescs;
	ShaderUniform::GetComputeShaderRootSigData(propertyPath, vars, passDescs);
	resultData.clear();
	resultData.reserve(32768);
	PutIn<uint>(resultData, (uint)vars.size());
	for (auto i = vars.begin(); i != vars.end(); ++i)
	{
		PutIn<vengine::string>(resultData, i->name);
		PutIn<ComputeShaderVariable::Type>(resultData, i->type);
		PutIn<uint>(resultData, i->tableSize);
		PutIn<uint>(resultData, i->registerPos);
		PutIn<uint>(resultData, i->space);
	}
	auto func = [&](vengine::string const& str, ProcessorData* data)->void
	{
		uint64_t fileSize;
		WaitChildProcess(data);
		//CreateChildProcess(command);
		//TODO
		//system(command.c_str());
		fileSize = 0;
		ifstream ifs(str.data(), ios::binary);
		if (!ifs) return;
		ifs.seekg(0, ios::end);
		fileSize = ifs.tellg();
		ifs.seekg(0, ios::beg);
		PutIn<uint64_t>(resultData, fileSize);
		if (fileSize == 0) return;
		size_t originSize = resultData.size();
		resultData.resize(fileSize + originSize);
		ifs.read(resultData.data() + originSize, fileSize);
	};
	PutIn<uint>(resultData, (uint)passDescs.size());
	vengine::string kernelCommand;

	ArrayList<ProcessorData> datas(passDescs.size());
	vengine::vector<vengine::string> strs(passDescs.size());

	for (auto i = passDescs.begin(); i != passDescs.end(); ++i)
	{
		strs[i.GetIndex()] = vengine::to_string(i.GetIndex()) + tempFilePath;
		if (compilerUsage == Compiler::FXC)
		{
			GenerateFXCCommand(
				fileName, i->name, strs[i.GetIndex()], i->macros, ShaderType::ComputeShader, kernelCommand);
		}
		else
		{
			GenerateDXCCommand(
				fileName, i->name, strs[i.GetIndex()], i->macros, ShaderType::ComputeShader, kernelCommand);
		}
		CreateChildProcess(kernelCommand, &datas[i.GetIndex()]);
		//func(kernelCommand, fileSize);
	}
	for (auto i = passDescs.begin(); i != passDescs.end(); ++i)
	{
		func(strs[i.GetIndex()], &datas[i.GetIndex()]);
	}
	for (auto i = strs.begin(); i != strs.end(); ++i)
		remove(i->c_str());
}
void getFiles(vengine::string path, vengine::vector<vengine::string>& files)
{
	uint64_t   hFile = 0;
	struct _finddata_t fileinfo;
	if ((hFile = _findfirst((path + "\\*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			if ((fileinfo.attrib & _A_SUBDIR))
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
					getFiles((path + "\\" + fileinfo.name), files);
			}
			else
			{
				files.push_back(path + "\\" + fileinfo.name);
			}
		} while (_findnext(hFile, &fileinfo) == 0);

		_findclose(hFile);
	}
}

typedef unsigned int uint;
void TryCreateDirectory(vengine::string& path)
{
	vengine::vector<uint> slashIndex;
	slashIndex.reserve(20);
	for (uint i = 0; i < path.length(); ++i)
	{
		if (path[i] == '/' || path[i] == '\\')
		{
			slashIndex.push_back(i);
			path[i] = '\\';
		}
	}
	if (slashIndex.empty()) return;
	vengine::string command;
	command.reserve(slashIndex[slashIndex.size() - 1] + 3);
	uint startIndex = 0;
	for (uint i = 0; i < slashIndex.size(); ++i)
	{
		uint value = slashIndex[i];
		for (uint x = startIndex; x < value; ++x)
		{
			command += path[x];
		}
		if (_access(command.data(), 0) == -1)
		{
			std::system(("md " + command).data());
		}
		startIndex = slashIndex[i];
	}
}

int main()
{
	start = " /nologo /enable_unbounded_descriptor_tables /all_resources_bound /Vd";
	shaderTypeCmd = " /T ";
	fullOptimize = " /O3";
	funcName = " /E ";
	output = " /Fo ";
	macro_compile = " /D ";

	ShaderUniform::Init();
	system("@echo off");
	vengine::string cmd;
	vengine::string sonCmd;
	vengine::string results;
	unique_ptr<ICompileIterator> dc;
	vengine::vector<Command>* cmds = nullptr;
	while (true)
	{
		cout << "Choose Compiling Mode: \n";
		cout << "  0: Compile Single File\n";
		cout << "  1: Compile Batched File\n";
		std::cin >> cmd;
		if (StringEqual(cmd, "exit"))
		{

			return 0;
		}
		else if (cmd.size() == 1) {

			if (cmd[0] == '0')
			{
				dc = std::unique_ptr<ICompileIterator>(new DirectCompile());
			}
			else if (cmd[0] == '1')
			{
				dc = std::unique_ptr<ICompileIterator>(new BatchCompile());
			}
			else
				dc = nullptr;

			if (dc)
			{
				dc->UpdateCommand();
				cout << "\n\n\n";
				cmds = &dc->GetCommand();
				if (cmds->empty()) continue;
			}
			else
			{
				continue;
			}
		EXECUTE:
			InitRegisteData();
			static vengine::string pathFolder = "CompileResult\\";

			atomic<uint64_t> counter = 0;
			if (cmds->size() > 1)
			{
				g_needCommandOutput = false;
				JobSystem* jobSys_MainGlobal;
				JobBucket* jobBucket_MainGlobal;
				uint threadCount = min(std::thread::hardware_concurrency() * 4, cmds->size());
				jobSys_MainGlobal = new JobSystem(threadCount);
				jobBucket_MainGlobal = jobSys_MainGlobal->GetJobBucket();

				for (size_t a = 0; a < cmds->size(); ++a)
				{
					Command* i = &(*cmds)[a];
					jobBucket_MainGlobal->GetTask([i, &counter]()->void
						{
							vengine::string outputData;
							vengine::string temp;
							temp.reserve(20);
							temp += "_";
							uint64_t tempNameIndex = ++counter;
							temp += std::to_string(tempNameIndex).c_str();
							temp += ".tmp";

							uint32_t maxSize = 0;
							if (i->isCompute)
							{
								CompileComputeShader(i->fileName, i->propertyFileName, temp, outputData);
							}
							else
							{
								CompileShader(i->fileName, i->propertyFileName, temp, outputData);
							}
							vengine::string filePath = pathFolder + i->fileName + ".cso";
							TryCreateDirectory(filePath);
							ofstream ofs(filePath.data(), ios::binary);
							ofs.write(outputData.data(), outputData.size());

						}, nullptr, 0);
				}
				cout << endl;

				jobSys_MainGlobal->ExecuteBucket(jobBucket_MainGlobal, 1);
				jobSys_MainGlobal->Wait();
				jobSys_MainGlobal->ReleaseJobBucket(jobBucket_MainGlobal);
				delete jobSys_MainGlobal;
			}
			else
			{
				g_needCommandOutput = true;
				vengine::string outputData;
				vengine::string temp = ".temp.cso";
				uint32_t maxSize = 0;
				Command* i = &(*cmds)[0];
				if (i->isCompute)
				{
					CompileComputeShader(i->fileName, i->propertyFileName, temp, outputData);
				}
				else
				{
					CompileShader(i->fileName, i->propertyFileName, temp, outputData);
				}
				vengine::string filePath = pathFolder + i->fileName + ".cso";
				TryCreateDirectory(filePath);
				ofstream ofs(filePath.data(), ios::binary);
				ofs.write(outputData.data(), outputData.size());
			}
			cout << "\n\n\n";
			cout << "Want to repeat the command again? Y for true\n";
			std::cin >> sonCmd;
			cout << "\n\n\n";
			if (sonCmd.length() == 1 && (sonCmd[0] == 'y' || sonCmd[0] == 'Y'))
			{
				goto EXECUTE;
			}
		}
	}
}
