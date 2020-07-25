#pragma once
#include "../Common/DLL.h"
class JobNode;
class JobBucket;
class __declspec(dllimport) JobHandle
{
	friend class JobBucket;
	friend class JobNode;
private:
	JobNode* node;
	JobHandle(JobNode* otherNode) : node(otherNode) {};
public:
	JobHandle() : node(nullptr) {}
	operator bool() const noexcept { return node; }
	JobHandle(const JobHandle& other)
	{
		node = other.node;
	}
	void Reset() { node = nullptr; }
	JobHandle& operator=(const JobHandle& other)
	{
		node = other.node;
		return *this;
	}
};