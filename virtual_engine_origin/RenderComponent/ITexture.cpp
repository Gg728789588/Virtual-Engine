#include "ITexture.h"
#include "../Singleton/Graphics.h"
#include "../Singleton/FrameResource.h"
ITexture::ITexture() {
	srvDescID = Graphics::GetDescHeapIndexFromPool();
}
ITexture::~ITexture()
{
	Graphics::ReturnDescHeapIndexToPool(srvDescID);

}