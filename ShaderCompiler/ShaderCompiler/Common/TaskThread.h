#pragma once
#include "TypeWiper.h"
#include <thread>
#include <mutex>
#include <condition_variable>
class TaskThread
{
private:
	std::thread thd;
	std::mutex mtx;
	std::mutex mainThreadMtx;
	std::condition_variable cv;
	std::condition_variable mainThreadCV;
	bool enabled;
	bool runNext;
	bool mainThreadLocked;
	void(*funcData)(void*);
	void* funcBody;
	static void RunThread(TaskThread* ptr);
	template <typename T>
	inline static void Run(void* ptr)
	{
		(*(T*)ptr)();
	}
public:
	TaskThread();
	void ExecuteNext();
	void Complete();
	~TaskThread();
	template <typename T>
	void SetFunctor(T& func)
	{
		funcData = Run<T>;
		funcBody = &func;
	}
};
