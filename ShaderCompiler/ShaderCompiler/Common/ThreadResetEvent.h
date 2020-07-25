#pragma once
#include <mutex>
#include <condition_variable>
#include <thread>
class ThreadResetEvent
{
private:
	std::condition_variable var;
	std::mutex mtx;
	bool isLocked;
public:
	ThreadResetEvent(bool isLockedInit) :
		isLocked(isLockedInit)
	{	}

	void Wait();
	void WaitAutoLock();
	void Lock();
	void Unlock();
};