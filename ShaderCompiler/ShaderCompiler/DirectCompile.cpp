#include "DirectCompile.h";
#include <fstream>
#include "Common/vstring.h"
using namespace std;
DirectCompile::DirectCompile()
{
}

vengine::vector<Command>& DirectCompile::GetCommand()
{
	return commands;
}

void DirectCompile::UpdateCommand()
{
	commands.clear();
	vengine::string fileName;
	cout << "Please Input the source shader file name: " << endl;
	cin >> fileName;
	Command c;
	{
		ifstream sourceIfs((fileName + ".hlsl").data());
		ifstream sourceCps((fileName + ".compute").data());
		if (sourceIfs)
		{
			c.fileName = fileName + ".hlsl";
			c.isCompute = false;
		}
		else if(sourceCps)
		{
			c.fileName = fileName + ".compute";
			c.isCompute = true;
		}
		else
		{
			return;
		}
	}
	
	c.propertyFileName = fileName + ".prop";
	commands.push_back(c);
}