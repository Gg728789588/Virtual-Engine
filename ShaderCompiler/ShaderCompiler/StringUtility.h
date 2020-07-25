#pragma once
#include "Common/vector.h"
#include <fstream>
#include "Common/vstring.h"
using uint = uint32_t;
void IndicesOf(const vengine::string& str, const vengine::string& sign, vengine::vector<uint>& v);
void IndicesOf(const vengine::string& str, char, vengine::vector<uint>& v);
void CutToLine(const vengine::string& str, vengine::vector<vengine::string>& lines);
void CutToLine(const char* str, int64_t size, vengine::vector<vengine::string>& lines);
void ReadLines(std::ifstream& ifs, vengine::vector<vengine::string>& lines);
int GetFirstIndexOf(const vengine::string& str, const vengine::string& sign);
int GetFirstIndexOf(const vengine::string& str, char sign);
void Split(const vengine::string& str, const vengine::string& sign, vengine::vector<vengine::string>& v);
void Split(const vengine::string& str, char sign, vengine::vector<vengine::string>& v);
int64_t GetNumberFromAttribute(const vengine::string& str, bool* findSign);
void GetDataFromBrackets(const vengine::string& str, vengine::string& result);
void ToLower(vengine::string& str);
void ToUpper(vengine::string& str);
int64_t StringToInt(const vengine::string& str);
double StringToFloat(const vengine::string& str);
bool StringEqual(const vengine::string& a, const vengine::string& b);
void RemoveCharFromString(vengine::string& str, char targetChar);
void RemoveCharsFromString(vengine::string& str, const std::initializer_list<const char>& targetChar);