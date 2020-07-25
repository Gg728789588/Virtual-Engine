#include "BatchCompile.h"
#include <iostream>
#include <fstream>
#include "StringUtility.h"
using namespace std;
BatchCompile::BatchCompile()
{
}
void BatchCompile::UpdateCommand()
{
	cout << "Please input batch look-up file: " << endl;
	cin >> lookUpFilePath;

}
vengine::vector<Command>& BatchCompile::GetCommand()
{
	Command c;
	commands.clear();

	ifstream ifs(lookUpFilePath.data());
	if (!ifs)
	{
		cout << "Look-up File Not Exists!" << endl;
		return commands;
	}
	vengine::vector<vengine::string> splitedCommands;
	vengine::vector<vengine::string> inputLines;
	ReadLines(ifs, inputLines);
	for (auto i = inputLines.begin(); i != inputLines.end(); ++i)
	{
		vengine::string& s = *i;
		std::ifstream vsIfs((s + ".hlsl").data());
		std::ifstream csIfs((s + ".compute").data());
		if (vsIfs)
		{
			c.isCompute = false;
			c.fileName = s + ".hlsl";
		}
		else if (csIfs)
		{
			c.isCompute = true;
			c.fileName = s + ".compute";
		}
		else continue;
		c.propertyFileName = s + ".prop";
		commands.push_back(c);
	}
	return commands;
}