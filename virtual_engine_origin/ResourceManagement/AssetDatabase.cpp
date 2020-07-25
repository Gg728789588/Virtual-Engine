#include "AssetDatabase.h"
#include "../Common/MObject.h"
#include "../RenderComponent/Mesh.h"
#include "../RenderComponent/Texture.h"
#include "AssetDatabase.h"
#include "../LogicComponent/World.h"
#include "../LogicComponent/Transform.h"
#include "../CJsonObject/CJsonObject.hpp"
#include "../RenderComponent/Light.h"
#include "../Utility/ThreadResetEvent.h"
#include "../Singleton/FrameResource.h"
AssetDatabase* AssetDatabase::current(nullptr);
using namespace neb;
ID3D12Device* AssetDatabase::device = nullptr;
namespace LoadingThreadFlags
{
	bool threadEnabled = true;
}
void AssetDatabase::LoadingThreadMainLogic()
{
	while (LoadingThreadFlags::threadEnabled)
	{
		{
			std::unique_lock<std::mutex> lck(current->loadingThreadMtx);
			while (current->shouldWaiting)
				current->cv.wait(lck);
			current->shouldWaiting = true;
		}
		//TODO
		//Load Logic Here!
	}
}

AssetDatabase::AssetDatabase(ID3D12Device* dev, IDXGIAdapter* adapter)
{
	splitCache.reserve(5);
	waitingSyncLock.reserve(20);
	this->device = dev;
	current = this;
	loadingThread = (std::thread*) & threadStorage;
	new (loadingThread) std::thread(LoadingThreadMainLogic);
	rootJsonObj = (CJsonObject*)&rootJsonObjStorage;
	if (!ReadJson("Resource/AssetDatabase.json", rootJsonObj))
	{

		rootJsonObj = nullptr;
	}
	vengine::string key, value;
	while (rootJsonObj->GetKey(key))
	{
		rootJsonObj->Get(key, value);
		vengine::string& v = value;
	}
	ReadJson("Data/JsonDirectory.json", jsonDirectory);
	if (!jsonDirectory)
	{
		throw "Json Not Found Exception!";
	}
}

AssetDatabase::~AssetDatabase()
{

	LoadingThreadFlags::threadEnabled = false;
	loadingThread->join();
	loadingThread->~thread();
	if (rootJsonObj)
		rootJsonObj->~CJsonObject();
}
#include "../Utility/StringUtility.h"
bool AssetDatabase::TryGetJson(const vengine::string& name, neb::CJsonObject* targetJsonObj)
{
	vengine::string path;
	StringUtil::Split(name, ' ', splitCache);
	if (!jsonDirectory->Get(*(splitCache.end() - 1), path)) return false;
	path = vengine::string("Data/") + path;
	std::ifstream ifs(path.c_str());
	if (!ifs) return false;
	ifs.seekg(0, std::ios::end);
	size_t len = ifs.tellg();
	ifs.seekg(0, 0);
	vengine::string s;
	s.resize(len);
	ifs.read((char*)s.data(), len);
	return targetJsonObj->Parse(s);
}