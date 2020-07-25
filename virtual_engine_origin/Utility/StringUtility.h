#include "../Common/vector.h"
#include <fstream>
#include "../Common/vstring.h"
typedef unsigned int uint;
namespace StringUtil
{
	void IndicesOf(const vengine::string& str, const vengine::string& sign, vengine::vector<uint>& v);
	void IndicesOf(const vengine::string& str, char, vengine::vector<uint>& v);
	void CutToLine(const vengine::string& str, vengine::vector<vengine::string>& lines);
	void CutToLine(const char* str, int64_t size, vengine::vector<vengine::string>& lines);
	void ReadLines(std::ifstream& ifs, vengine::vector<vengine::string>& lines);
	int GetFirstIndexOf(const vengine::string& str, const vengine::string& sign);
	int GetFirstIndexOf(const vengine::string& str, char sign);
	void Split(const vengine::string& str, const vengine::string& sign, vengine::vector<vengine::string>& v);
	void Split(const vengine::string& str, char sign, vengine::vector<vengine::string>& v);
	void GetDataFromAttribute(const vengine::string& str, vengine::string& result);
	void GetDataFromBrackets(const vengine::string& str, vengine::string& result);
	int StringToInteger(const vengine::string& str);
	void ToLower(vengine::string& str);
	void ToUpper(vengine::string& str);
}