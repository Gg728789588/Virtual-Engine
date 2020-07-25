#pragma once
#include <iostream>
#include "Common/vstring.h"
#include "Common/vector.h"
enum class ShaderType : uint16_t
{
	ComputeShader = 0,
	VertexShader = 1,
	PixelShader = 2,
	HullShader = 3,
	DomainShader = 4,
	GeometryShader = 5
};

struct Command
{
	vengine::string fileName;
	vengine::string propertyFileName;
	bool isCompute;
};
class ICompileIterator
{
public:
	virtual vengine::vector<Command>& GetCommand() = 0;
	virtual void UpdateCommand() = 0;
	virtual ~ICompileIterator() {}
};