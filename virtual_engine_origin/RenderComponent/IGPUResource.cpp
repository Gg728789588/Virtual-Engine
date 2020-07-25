#include "IGPUResource.h"
#include "../Singleton/FrameResource.h"
void IGPUResource::DelayReleaseAfterFrame()
{
	releaseAfterFrame = true;
}
IGPUResource::~IGPUResource()
{
	if (releaseAfterFrame)
	{
		for (auto ite = FrameResource::mFrameResources.begin(); ite != FrameResource::mFrameResources.end(); ++ite)
		{
			if (*ite) (*ite)->ReleaseResourceAfterFlush(Resource);
		}
	}
}

void IGPUResource::ReleaseAfterFrame(FrameResource* resource) const
{
	if (!releaseAfterFrame) resource->ReleaseResourceAfterFlush(Resource);
}