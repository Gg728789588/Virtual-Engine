#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/MetaLib.h"
#include "../CJsonObject/CJsonObject.hpp"
#include <mutex>
#include <thread>
#include <condition_variable>
#include "../Common/MetaLib.h"
#include "../Common/ArrayList.h"
#include "../Common/HashMap.h"
#include "../Common/Runnable.h"
#include "../Utility/ThreadResetEvent.h"

class FrameResource;
class AssetDatabase final
{

private:
	static AssetDatabase* current;
	std::mutex mtx;
	Storage<std::thread, 1> threadStorage;
	std::thread* loadingThread;

	ArrayList<ThreadResetEvent*> waitingSyncLock;
	
	bool shouldWaiting = false;
	std::mutex loadingThreadMtx;
	std::condition_variable cv;
	static ID3D12Device* device;
	AssetDatabase(ID3D12Device* device, IDXGIAdapter* adapter);
	~AssetDatabase();
	Storage<neb::CJsonObject, 1> rootJsonObjStorage;
	neb::CJsonObject* rootJsonObj;
	StackObject<neb::CJsonObject, true> jsonDirectory;
	vengine::vector<vengine::string> splitCache;
	
public:
	static void LoadingThreadMainLogic();

	static AssetDatabase* GetInstance()
	{
		return current;
	}
	static AssetDatabase* CreateInstance(ID3D12Device* device, IDXGIAdapter* adapter)
	{
		if (current) return current;
		new AssetDatabase(device, adapter);
		return current;
	}
	static void DestroyInstance()
	{
		if (current)
		{
			delete current;
			current = nullptr;
		}
	}

	bool TryGetJson(const vengine::string& name, neb::CJsonObject* targetJsonObj);
};