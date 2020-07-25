#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/MetaLib.h"
#include "../Common/ArrayList.h"
#include "UploadBuffer.h"
struct ConstBufferElement
{
	UploadBuffer const* const buffer;
	UINT const element;
	ConstBufferElement() : buffer(nullptr),
		element(0)
	{	}
	void operator=(const ConstBufferElement& ele)
	{
		new (this)ConstBufferElement(ele);
	}
	ConstBufferElement(const ConstBufferElement& ele) :
		buffer(ele.buffer),
		element(ele.element) {}
	ConstBufferElement(UploadBuffer const* const buffer, UINT const element) :
		buffer(buffer), element(element) {}
};
class CBufferPool
{
private:
	vengine::vector<ObjectPtr<UploadBuffer>> arr;
	ArrayList<ConstBufferElement> poolValue;
	UINT capacity;
	UINT stride;
	bool isConstantBuffer;
	void Add(ID3D12Device* device);
public:
	CBufferPool(UINT stride, UINT initCapacity, bool isConstantBuffer = true);
	~CBufferPool();
	ConstBufferElement Get(ID3D12Device* device);
	void Return(const ConstBufferElement& target);
};