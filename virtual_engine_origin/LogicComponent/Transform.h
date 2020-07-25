#pragma once
#include <DirectXMath.h>
#include "../Common/MObject.h"
#include "../LogicComponent/Component.h"
#include "../Common/RandomVector.h"
#include "../Common/ArrayList.h"
class World;
class JobHandle;
class JobBucket;
class TransformMoveStruct;

namespace neb
{
	class CJsonObject;
}
struct TransformData
{
	float3 up;
	float3 forward;
	float3 right;
	float3 localScale;
	float3 position;
	bool isWorldMovable;
};
class Transform final : public MObject
{
	friend class Scene;
	friend class Component;
	friend class TransformMoveStruct;
	friend class World;
private:
	static std::mutex transMutex;
	static RandomVector<TransformData, true> randVec;
	static ArrayList<JobHandle> moveWorldHandles;
	RandomVector<ObjectPtr<Component>> allComponents;
	int worldIndex;
	uint vectorPos;
	Transform(const neb::CJsonObject& path, ObjectPtr<Transform>& targetPtr, bool isWorldMovable = true);
	Transform(ObjectPtr<Transform>& targetPtr, bool isWorldMovable = true);
	Transform(ObjectPtr<Transform>& targetPtr, const Math::Vector3& position, const Math::Vector4& rotation, const Math::Vector3& localScale, bool isWorldMovable = true);
	void Dispose();
	static void MoveTheWorld(int3* worldBlockIndexPtr, const int3& moveBlock, const double3& moveDirection, JobBucket* bucket, ArrayList<JobHandle>& jobHandles);
public:
	uint GetComponentCount() const
	{
		return allComponents.Length();
	}
	static ObjectPtr<Transform> GetTransform(bool isWorldMovable = true);
	static ObjectPtr<Transform> GetTransform(const neb::CJsonObject& path, bool isWorldMovable = true);
	static ObjectPtr<Transform> GetTransform(
		const Math::Vector3& position, const Math::Vector4& rotation, const Math::Vector3& localScale,
		bool isWorldMovable = true);
	float3 GetPosition() const { return randVec[vectorPos].position; }
	float3 GetForward() const { return randVec[vectorPos].forward; }
	float3 GetRight() const { return randVec[vectorPos].right; }
	float3 GetUp() const { return randVec[vectorPos].up; }
	float3 GetLocalScale() const { return randVec[vectorPos].localScale; }
	void SetRight(const Math::Vector3& right);
	void SetUp(const Math::Vector3& up);
	void SetForward(const Math::Vector3& forward);
	void SetRotation(const Math::Vector4& quaternion);
	void SetPosition(const float3& position);
	void SetLocalScale(const float3& localScale);
	Math::Matrix4 GetLocalToWorldMatrixCPU() const;
	Math::Matrix4 GetLocalToWorldMatrixGPU() const;
	Math::Matrix4 GetWorldToLocalMatrixCPU() const;
	Math::Matrix4 GetWorldToLocalMatrixGPU() const;
	~Transform();
	static void DisposeTransform(ObjectPtr<Transform>& trans);
	
};
