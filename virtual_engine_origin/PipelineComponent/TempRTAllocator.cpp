#include "TempRTAllocator.h"
#include "../Singleton/ShaderID.h"
TempRTAllocator::TempRTAllocator() : 
	waitingRT(37)
{
}

bool ResourceDescriptor::operator==(const ResourceDescriptor& res) const
{
	if (type != res.type) return false;
	if (type == ResourceDescriptor::ResourceType_RenderTexture)
		return res.rtDesc == rtDesc;
	else
	{
		return res.sbufDesc.elementCount == sbufDesc.elementCount &&
			res.sbufDesc.stride == sbufDesc.stride;
	}
}

bool ResourceDescriptor::operator!=(const ResourceDescriptor& res) const
{
	return !operator==(res);
}

TempRTAllocator::~TempRTAllocator()
{
	waitingRT.IterateAll([](uint i, const ResourceDescriptor& desc, ArrayList<TempRTData>*& v)->void
		{
			for (int j = 0; j < v->size(); ++j)
				delete (*v)[j].rt;
			delete v;
		});
}
MObject* TempRTAllocator::GetTempResource(ID3D12Device* device, UINT id, ResourceDescriptor& descriptor)
{

	ArrayList<TempRTData>* datas = nullptr;
	ArrayList<TempRTData>*& datasPtr = waitingRT[descriptor];
	if (&datasPtr == nullptr)
	{
		datas = new ArrayList<TempRTData>();
		datas->reserve(10);
		waitingRT.Insert(descriptor, datas);
	}
	else
		datas = datasPtr;
	if (datas->size() > 0)
	{
		auto dataIte = datas->end() - 1;
		TempRTData& data = *dataIte;
		UsingTempRT usingRTData;
		usingRTData.rt = data.rt;
		usingRTData.desc = descriptor;
		usingRT.Insert(id, usingRTData);
		MObject* result = data.rt;
		datas->erase(dataIte);
		return result;
	}
	else
	{
		UsingTempRT usingRTData;
		if (descriptor.type == ResourceDescriptor::ResourceType_RenderTexture)
		{
			RenderTextureDescriptor& rtDesc = descriptor.rtDesc;
			usingRTData.rt = new RenderTexture(device, rtDesc.width, rtDesc.height, rtDesc.rtFormat,rtDesc.type, rtDesc.depthSlice, 0);
			/*
#if defined(DEBUG) || defined(_DEBUG)
			auto&& str = ShaderID::IDToProperty(id);
			std::wstring wstr;
			wstr.resize(str.size());
			for (uint i = 0; i < wstr.size(); ++i)
			{
				wstr[i] = str[i];
			}
			((RenderTexture*)usingRTData.rt)->SetName(wstr);
#endif*/
		}
		else
		{
			StructuredBufferElement& sbufDesc = descriptor.sbufDesc;
			usingRTData.rt = new StructuredBuffer(device, &sbufDesc, 1);
		}
		usingRTData.desc = descriptor;
		usingRT.Insert(id, usingRTData);
		return usingRTData.rt;
	}
}
bool TempRTAllocator::Contains(UINT id)
{
	return usingRT.Contains(id);
}
MObject* TempRTAllocator::GetUsingRenderTexture(UINT id)
{
	auto ite = usingRT.Find(id);
	if (ite)
	{

		return ite.Value().rt;
	}
	return nullptr;
}

void TempRTAllocator::ReleaseRenderTexutre(UINT id)
{
	auto ite = usingRT.Find(id);
	if (ite)
	{
		ArrayList<TempRTData>*& datasPtr = waitingRT[ite.Value().desc];
		
		if (&datasPtr != nullptr)
		{
			ArrayList<TempRTData>* data = datasPtr;
			TempRTData rtData;
			rtData.rt = ite.Value().rt;
			rtData.containedFrame = 0;
			data->push_back(rtData);
			int size = data->size();
			size = data->size();
		}
		usingRT.Remove(ite);
	}
}

void TempRTAllocator::CumulateReleaseAfterFrame()
{
	waitingRT.IterateAll([](uint i, ResourceDescriptor const& res, ArrayList<TempRTData>*& data)->void
		{
			for (UINT j = 0; j < data->size(); ++j)
			{
				TempRTData& d = (*data)[j];
				d.containedFrame++;
				if (d.containedFrame >= 4)
				{
					MObject* ptr = d.rt;
					(*data)[j] = (*data)[data->size() - 1];
					data->erase(data->end() - 1);
					j--;
					delete ptr;
				}
			}
		});
}