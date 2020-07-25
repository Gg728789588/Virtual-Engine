#pragma once
#include "ICompileIterator.h"

class DirectCompile : public ICompileIterator
{
	vengine::vector<Command> commands;
public:
	DirectCompile();
	virtual void UpdateCommand();
	virtual vengine::vector<Command>& GetCommand();
};