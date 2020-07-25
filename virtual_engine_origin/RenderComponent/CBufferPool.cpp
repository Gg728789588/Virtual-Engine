#include "CBufferPool.h" 
#include <math.h>
void CBufferPool::Add(ID3D12Device* device)
{
	ObjectPtr<UploadBuffer>& newBuffer = arr.emplace_back(ObjectPtr<UploadBuffer>::NewObject(device, capacity, isConstantBuffer, stride));
	UploadBuffer* nb = newBuffer;
	for (uint i = 0; i < capacity; ++i)
	{
		poolValue.push_back(ConstBufferElement(nb, i));
	}
}

CBufferPool::CBufferPool(UINT stride, UINT initCapacity, bool isConstantBuffer) :
	capacity(initCapacity),
	stride(stride), isConstantBuffer(isConstantBuffer)
{
	poolValue.reserve(initCapacity);
	arr.reserve(10);
}

CBufferPool::~CBufferPool()
{
}

ConstBufferElement CBufferPool::Get(ID3D12Device* device)
{
	if (poolValue.empty())
	{
		Add(device);
	}
	auto ite = poolValue.end() - 1;
	ConstBufferElement pa = *ite;
	poolValue.erase(ite);
	return pa;

}

void CBufferPool::Return(const ConstBufferElement& target)
{
	poolValue.push_back(target);
}
