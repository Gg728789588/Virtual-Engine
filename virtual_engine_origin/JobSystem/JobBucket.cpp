#ifdef DLL_EXPORT
#include "JobBucket.h"
#include "JobNode.h"
#include "JobSystem.h"
JobBucket::JobBucket(JobSystem* sys) noexcept : 
	sys(sys)
{
	jobNodesVec.reserve(20);
}


JobHandle JobBucket::GetTask(JobHandle const* dependedJobs, uint32_t dependCount, const FunctorData& funcData, void* funcPtr, uint64_t funcSize)
{
	JobNode* node = sys->jobNodePool.New();
	allJobNodeCount++;
	node->Create(funcData, funcPtr, sys, dependedJobs, funcSize, dependCount);
	if (dependCount == 0) jobNodesVec.push_back(node);
	return JobHandle(node);
}

JobHandle JobBucket::GetFence(JobHandle const* dependedJobs, uint32_t dependCount)
{
	JobNode* node = sys->jobNodePool.New();
	allJobNodeCount++;
	node->CreateEmpty(sys, dependedJobs, dependCount);
	if (dependCount == 0) jobNodesVec.push_back(node);
	return JobHandle(node);
}
#endif