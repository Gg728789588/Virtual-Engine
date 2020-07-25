#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
class FrameResource;
class IGPUResource : public MObject
{
protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
private:
	bool releaseAfterFrame = false;
public:
	void DelayReleaseAfterFrame();
	void ReleaseAfterFrame(FrameResource* resource) const;
	virtual ~IGPUResource();
	ID3D12Resource* GetResource() const
	{
		return Resource.Get();
	}
	const Microsoft::WRL::ComPtr<ID3D12Resource>& GetResourcePtr() const
	{
		return Resource;
	}
};