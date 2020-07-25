#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include "ConcurrentQueue.h"
#include "../Common/Pool.h"
#include <DirectXMath.h>
#include "../Common/TypeWiper.h"
#include "../Common/MetaLib.h"
#include "../Common/DLL.h"
class JobHandle;
class JobThreadRunnable;
class JobBucket;
class JobSystem;
typedef uint32_t uint;
class __declspec(dllimport) JobNode
{
	friend class JobBucket;
	friend class JobSystem;
	friend class JobHandle;
	friend class JobThreadRunnable;
private:
	StackObject<ArrayList<JobNode*>> dependingEvent;
	void* ptr;
	void(*destructorFunc)(void*) = nullptr;
	void(*executeFunc)(void*);
	std::mutex* threadMtx;
	std::atomic<uint32_t> targetDepending;
	bool dependedEventInitialized = false;
	void Create(const FunctorData& funcData, void* funcPtr, JobSystem* sys, JobHandle const* dependedJobs, uint64_t size, uint dependCount);
	void CreateEmpty(JobSystem* sys, JobHandle const* dependedJobs, uint dependCount);
	JobNode* Execute(ConcurrentQueue<JobNode*>& taskList, std::condition_variable& cv);
public:
	void Reset();
	void Dispose();
	~JobNode();
};