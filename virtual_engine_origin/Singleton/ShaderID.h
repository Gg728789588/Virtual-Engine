#pragma once
#include <mutex>
#include "../Common/HashMap.h"
#include "../Common/vstring.h"
#include "../Common/vector.h"
class ShaderID
{
	static const unsigned int INIT_CAPACITY = 100;
	static unsigned int currentCount;
	static HashMap<vengine::string, unsigned int> allShaderIDs;
	static vengine::vector<vengine::string> idNames;
	static unsigned int mPerCameraBuffer;
	static unsigned int mPerMaterialBuffer;
	static unsigned int mPerObjectBuffer;
	static unsigned int mainTex;
	static unsigned int params;
	static std::mutex mtx;
public:
	static void Init();
	static unsigned int GetPerCameraBufferID() { return mPerCameraBuffer; }
	static unsigned int GetPerMaterialBufferID() { return mPerMaterialBuffer; }
	static unsigned int GetPerObjectBufferID() { return mPerObjectBuffer; }
	static unsigned int GetMainTex() { return mainTex; }
	static unsigned int PropertyToID(const vengine::string& str);
	static vengine::string const& IDToProperty(uint32_t id);
	static unsigned int GetParams() { return params; }

};

