#ifdef DLL_EXPORT
#include "JobNode.h"
#include "JobSystem.h"
#include "JobBucket.h"
JobNode::~JobNode()
{
	Dispose();
	if (dependedEventInitialized)
		dependingEvent.Delete();
}

JobNode* JobNode::Execute(ConcurrentQueue<JobNode*>& taskList, std::condition_variable& cv)
{
	if (executeFunc) executeFunc(ptr);
	auto ite = dependingEvent->begin();
	JobNode* nextNode = nullptr;
	while (ite != dependingEvent->end())
	{
		JobNode* node = *ite;
		uint32_t dependingCount = --node->targetDepending;
		if (dependingCount == 0)
		{
			nextNode = node;
			++ite;
			break;
		}
		++ite;
	}
	for (; ite != dependingEvent->end(); ++ite)
	{
		JobNode* node = *ite;
		uint32_t dependingCount = --node->targetDepending;
		if (dependingCount == 0)
		{
			taskList.Push(node);
			{
				std::lock_guard<std::mutex> lck(*threadMtx);
				cv.notify_one();
			}
		}
	}
	return nextNode;
}
/*
void JobNode::Precede(JobNode* depending)
{
	depending->targetDepending++;
	dependingEvent->emplace_back(depending);
	if (depending->vecIndex >= 0)
	{
		JobBucket* bucket = depending->bucket;
		auto lastIte = bucket->jobNodesVec.end() - 1;
		auto indIte = bucket->jobNodesVec.begin() + depending->vecIndex;
		*indIte = *lastIte;
		(*indIte)->vecIndex = depending->vecIndex;
		bucket->jobNodesVec.erase(lastIte);
		depending->vecIndex = -1;
	}
}
*/


void JobNode::Create(const FunctorData& funcData, void* funcPtr, JobSystem* sys, JobHandle const* dependedJobs, uint64_t size, uint dependCount)
{
	std::mutex* threadMtx = &sys->threadMtx;
	targetDepending = dependCount;
	this->threadMtx = threadMtx;
	if (funcData.constructor)
	{
		ptr = sys->AllocFuncMemory(size);;
		funcData.constructor(ptr, funcPtr);
	}
	destructorFunc = funcData.disposer;
	executeFunc = funcData.run;
	for (uint i = 0; i < dependCount; ++i)
	{
		auto node = dependedJobs[i].node;
		node->dependingEvent->push_back(this);
	}
}

void JobNode::CreateEmpty(JobSystem* sys, JobHandle const* dependedJobs, uint dependCount)
{
	std::mutex* threadMtx = &sys->threadMtx;
	targetDepending = dependCount;
	this->threadMtx = threadMtx;
	destructorFunc = nullptr;
	executeFunc = nullptr;
	for (uint i = 0; i < dependCount; ++i)
	{
		auto node = dependedJobs[i].node;
		node->dependingEvent->push_back(this);
	}
}

void JobNode::Reset()
{
	if (!dependedEventInitialized)
	{
		dependedEventInitialized = true;
		dependingEvent.New();
		dependingEvent->reserve(8);
	}
}
void JobNode::Dispose()
{
	if (destructorFunc != nullptr)
	{
		destructorFunc(ptr);
		destructorFunc = nullptr;
	}
	if (dependedEventInitialized)
	{
		dependingEvent->clear();
	}
}
#endif