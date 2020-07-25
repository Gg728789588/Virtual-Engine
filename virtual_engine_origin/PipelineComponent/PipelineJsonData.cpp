#include "PipelineJsonData.h"
#include "../CJsonObject/CJsonObject.hpp"
#include "../ResourceManagement/AssetDatabase.h"
using namespace neb;
void PipelineJsonData::GetPipelineIndex(uint* indices, uint offset, const std::type_info& typeData)
{
	vengine::string na = typeData.name();
	auto ite = typeToIndex.Find(na);
	if (ite)
	{
		indices[offset] = ite.Value();
	}
	else
	{
		CJsonObject* jsonPtr = jsonPool.New();
		AssetDatabase* instance = AssetDatabase::GetInstance();
		if (instance->TryGetJson(na, jsonPtr))
		{
			jsonObjs.Insert(
				componentIndex, ObjectPtr<CJsonObject>::MakePtrNoMemoryFree(jsonPtr));
		}
		else
		{
			jsonPool.Delete(jsonPtr);
		}
		typeToIndex.Insert(na, componentIndex);
		indices[offset] = componentIndex;
		componentIndex++;
	}
}

CJsonObject* PipelineJsonData::GetJsonObject(uint index) const
{
	auto ite = jsonObjs.Find(index);
	if (!ite ) return nullptr;
	return ite.Value();
}

void PipelineJsonData::ForceUpdateAllJson()
{
	AssetDatabase* instance = AssetDatabase::GetInstance();
	auto func = [&](uint i, const vengine::string& key, uint& value)
	{
		auto jsonIte = jsonObjs.Find(value);
		if (!jsonIte)
		{
			CJsonObject* jsonPtr = jsonPool.New();

			if (instance->TryGetJson(key, jsonPtr))
			{
				jsonObjs.Insert(
					componentIndex, ObjectPtr<CJsonObject>::MakePtrNoMemoryFree(jsonPtr));
			}
			else
			{
				jsonPool.Delete(jsonPtr);
			}
		}
		else
		{
			instance->TryGetJson(key, jsonIte.Value());
		}
	};
	typeToIndex.IterateAll(func);
}

PipelineJsonData::PipelineJsonData() :
	jsonPool(64), typeToIndex(23),
	jsonObjs(23)
{
}

PipelineJsonData::~PipelineJsonData()
{
	jsonObjs.Clear();
}
