#pragma once
#include <mutex>
#include "../Common/vector.h"
#include <atomic>
class JobNode;
class VectorPool
{
private:
	struct Array
	{
		vengine::vector<JobNode*>** objs;
		std::atomic<int64_t> count;
		int64_t capacity;
	};

	Array unusedObjects[2];
	std::mutex mtx;
	bool objectSwitcher = true;
public:
	void UpdateSwitcher();
	void Delete(vengine::vector<JobNode*>* targetPtr);

	vengine::vector<JobNode*>* New();

	VectorPool(unsigned int initCapacity);
	~VectorPool();
};