#pragma once


#include <initializer_list>
#include "JobHandle.h"
#include "../Common/Pool.h"
#include "../Common/ArrayList.h"
#include "../Common/TypeWiper.h"
#include "../Common/DLL.h"
class JobSystem;
class JobThreadRunnable;
class JobNode;
class __declspec(dllimport) JobBucket
{
	friend class JobSystem;
	friend class JobNode;
	friend class JobHandle;
	friend class JobThreadRunnable;
private:
	ArrayList<JobNode*> jobNodesVec;
	uint32_t allJobNodeCount = 0;
	JobSystem* sys = nullptr;
	JobBucket(JobSystem* sys) noexcept;
	~JobBucket() noexcept{}
	JobHandle GetTask(JobHandle const* dependedJobs, uint32_t dependCount, const FunctorData& funcData, void* funcPtr, uint64_t funcSize);
public:
	template <typename T>
	constexpr JobHandle GetTask(JobHandle const* dependedJobs, uint32_t dependCount, const T& func)
	{
		using Func = typename std::remove_cvref_t<T>;
		FunctorData funcData = GetFunctor<Func>();
		return GetTask(dependedJobs, dependCount, funcData, (Func*)(&func), sizeof(Func));
	}
	template <typename T>
	constexpr JobHandle GetTask(const std::initializer_list<JobHandle>& handles, const T& func)
	{
		return GetTask<T>(handles.begin(), handles.size(), func);
	}
	JobHandle GetFence(JobHandle const* dependedJobs, uint32_t dependCount);
	JobHandle GetFence(std::initializer_list<JobHandle> const& dependedJobs)
	{
		return GetFence(dependedJobs.begin(), dependedJobs.size());
	}
};