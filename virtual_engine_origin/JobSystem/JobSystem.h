#pragma once

#include "../Common/Pool.h"
#include "ConcurrentQueue.h"
#include <atomic>
#include <stdint.h>
#include "../Utility/MemoryBuddyAllocator.h"
#include "../Common/MetaLib.h"
#include "../Common/DLL.h"
class JobNode;
class JobBucket;
class JobThreadRunnable;

class __declspec(dllimport) JobSystem
{
	friend class JobBucket;
	friend class JobThreadRunnable;
	friend class JobNode;
private:
	MemoryBuddyAllocator __memAllocator;
	std::mutex threadMtx;
	void UpdateNewBucket();
	uint32_t mThreadCount;
	JobPool<JobNode> jobNodePool;
	ConcurrentQueue<JobNode*> executingNode;
	ArrayList<std::thread*> allThreads;
	ArrayList<void*> allocatedMemory[2];
	std::atomic<int32_t> bucketMissionCount;
	uint32_t currentBucketPos;
	ArrayList<JobBucket*> buckets;
	std::condition_variable cv;
	ArrayList<JobBucket*> usedBuckets;
	ArrayList<JobBucket*> releasedBuckets;
	std::mutex mainThreadWaitMutex;
	std::condition_variable mainThreadWaitCV;
	bool mainThreadFinished;
	bool JobSystemInitialized = true;
	bool jobSystemStart = false;
	bool allocatorSwitcher = false;
	void* AllocFuncMemory(uint64 size);
	void FreeAllMemory();
public:
	JobSystem(uint32_t threadCount) noexcept;
	void ExecuteBucket(JobBucket** bucket, uint32_t bucketCount);
	void ExecuteBucket(JobBucket* bucket, uint32_t bucketCount);
	void Wait();
	~JobSystem() noexcept;
	JobBucket* GetJobBucket();
	void ReleaseJobBucket(JobBucket* node);
};

