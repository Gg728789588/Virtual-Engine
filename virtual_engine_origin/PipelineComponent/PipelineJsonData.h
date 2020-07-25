#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../Common/MetaLib.h"
#include "../Common/Pool.h"
#include "../Common/HashMap.h"
namespace neb
{
	class CJsonObject;
}
class PipelineJsonData
{
private:
	uint componentIndex = 0;
	HashMap<vengine::string, uint> typeToIndex;
	Pool<neb::CJsonObject> jsonPool;
	HashMap<uint, ObjectPtr<neb::CJsonObject>> jsonObjs;
	void GetPipelineIndex(uint* indices, uint offset, const std::type_info& typeData);
	template <typename Array, typename T>
	constexpr void TypeToIndexCollection(Array& arr, uint& idx)
	{
		GetPipelineIndex(arr.data(), idx, typeid(T));
		idx++;
	}
public:
	void ForceUpdateAllJson();
	PipelineJsonData();
	~PipelineJsonData();
	neb::CJsonObject* GetJsonObject(uint index) const;
	template <typename T>
	constexpr uint TypeToIndex()
	{
		uint result;
		GetPipelineIndex(&result, 0, typeid(T));
		return result;
	}
	template<typename ... Args>
	constexpr std::array<uint, sizeof...(Args)> TypesToIndices()
	{
		using ArrayType = typename std::array<uint, sizeof...(Args)>;
		ArrayType arr;
		uint idx = 0;
		char c[] = { (TypeToIndexCollection<ArrayType, Args>(arr, idx), 0)... };
		return arr;
	}

};

