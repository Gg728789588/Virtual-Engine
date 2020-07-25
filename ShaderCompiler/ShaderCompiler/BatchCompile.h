#pragma once
#include "ICompileIterator.h"
class BatchCompile : public ICompileIterator
{
private:
	vengine::vector<Command> commands;
	vengine::string lookUpFilePath;
public:
	BatchCompile();
	virtual void UpdateCommand();
	virtual vengine::vector<Command>& GetCommand();
};