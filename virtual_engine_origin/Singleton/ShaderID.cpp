#include "ShaderID.h"
HashMap<vengine::string, unsigned int> ShaderID::allShaderIDs(INIT_CAPACITY);
vengine::vector<vengine::string> ShaderID::idNames;
unsigned int ShaderID::currentCount = 0;
unsigned int ShaderID::mPerCameraBuffer = 0;
unsigned int ShaderID::mPerMaterialBuffer = 0;
unsigned int ShaderID::mPerObjectBuffer = 0;
unsigned int ShaderID::mainTex = 0;
unsigned int ShaderID::params = 0;
std::mutex ShaderID::mtx;
unsigned int ShaderID::PropertyToID(const vengine::string& str)
{
	mtx.lock();
	auto ite = allShaderIDs.Find(str);
	if (!ite)
	{
		unsigned int value = currentCount;
		allShaderIDs.Insert(str, currentCount);
		idNames.push_back(str);
		++currentCount;
		mtx.unlock();
		return value;
	}
	else
	{
		mtx.unlock();
		return ite.Value();
	}
}

vengine::string const& ShaderID::IDToProperty(uint32_t id)
{
	return idNames[id];
}

void ShaderID::Init()
{
	idNames.reserve(INIT_CAPACITY);
	mPerCameraBuffer = PropertyToID("Per_Camera_Buffer");
	mPerMaterialBuffer = PropertyToID("Per_Material_Buffer");
	mPerObjectBuffer = PropertyToID("Per_Object_Buffer");
	mainTex = PropertyToID("_MainTex");
	params = PropertyToID("Params");
}