#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include "Transform.h"
#include "World.h"
#include "../Common/MetaLib.h"
#include "../Common/RandomVector.h"
#include "../JobSystem/JobInclude.h"
#include "../Common/Runnable.h"
#include "../CJsonObject/CJsonObject.hpp"
RandomVector<TransformData, true> Transform::randVec(100);
std::mutex Transform::transMutex;
ArrayList<JobHandle> Transform::moveWorldHandles;
using namespace Math;
using namespace neb;
void Transform::SetRotation(const Math::Vector4& quaternion)
{
	Matrix4 rotationMatrix = XMMatrixRotationQuaternion(quaternion);
	std::lock_guard<std::mutex> lck(transMutex);
	float3 right, up, forward;
	right = normalize((Vector4&)rotationMatrix[0]);
	up = normalize((Vector4&)rotationMatrix[1]);
	forward = normalize((Vector4&)rotationMatrix[2]);
	TransformData& data = randVec[vectorPos];
	data.right = right;
	data.up = up;
	data.forward = forward;
}
struct TransformMoveStruct
{
	uint start, end;
	double3 moveDirection;
	void operator()()
	{
		for (uint j = start; j < end; ++j)
		{
			auto& r = Transform::randVec[j];
			if (r.isWorldMovable) {
				r.position.x += moveDirection.x;
				r.position.y += moveDirection.y;
				r.position.z += moveDirection.z;
			}
		}
	}
};
void Transform::MoveTheWorld(int3* worldBlockIndexPtr, const int3& moveBlock, const double3& moveDirection, JobBucket* bucket, ArrayList<JobHandle>& jobHandles)
{
	JobHandle lockHandle = bucket->GetTask(nullptr, 0, [=]()->void
		{
			transMutex.lock();
			*worldBlockIndexPtr += moveBlock;
		});
	moveWorldHandles.clear();
	uint jobCount = randVec.Length() / 500;
	uint start = 0;
	uint end = 0;
	TransformMoveStruct moveStruct;
	moveStruct.moveDirection = moveDirection;
	for (uint i = 0; i < jobCount; ++i)
	{
		start = 500 * i;
		end = 500 * (i + 1);
		moveStruct.start = start;
		moveStruct.end = end;

		moveWorldHandles.push_back(bucket->GetTask(&lockHandle, 1, moveStruct));
	}
	start = jobCount * 500;
	end = randVec.Length();
	if (start < end)
	{
		moveStruct.start = start;
		moveStruct.end = end;
		moveWorldHandles.push_back(bucket->GetTask(&lockHandle, 1, moveStruct));
	}
	jobHandles.push_back(bucket->GetTask(moveWorldHandles.data(), moveWorldHandles.size(), []()->void
		{
			transMutex.unlock();
		}));
	/*
	for (uint i = 0; i < randVec.Length(); ++i)
	{
		auto& r = randVec[i];
		if (r.isWorldMovable)
			r.position += moveDirection;
	}
	*/
}
void Transform::SetRight(const Math::Vector3& right)
{
	std::lock_guard<std::mutex> lck(transMutex);
	TransformData& data = randVec[vectorPos];
	data.right = right;
}
void Transform::SetUp(const Math::Vector3& up)
{
	std::lock_guard<std::mutex> lck(transMutex);
	TransformData& data = randVec[vectorPos];
	data.up = up;
}
void Transform::SetForward(const Math::Vector3& forward)
{
	std::lock_guard<std::mutex> lck(transMutex);
	TransformData& data = randVec[vectorPos];
	data.forward = forward;
}
void Transform::SetPosition(const XMFLOAT3& position)
{
	std::lock_guard<std::mutex> lck(transMutex);
	randVec[vectorPos].position = position;
}
Transform::Transform(const CJsonObject& jsonObj, ObjectPtr<Transform>& targetPtr, bool isWorldMovable)
{
	World* world = World::GetInstance();
	if (world != nullptr)
	{
		std::lock_guard<std::mutex> lck(world->mtx);
		targetPtr = ObjectPtr<Transform>::MakePtr(this);
		world->allTransformsPtr.Add(targetPtr, (uint*)&worldIndex);
	}
	else
	{
		worldIndex = -1;
	}
	vengine::string s;
	double3 pos = { 0,0,0 };
	float4 rot = { 0,0,0,1 };
	float3 scale = { 1,1,1 };
	if (jsonObj.Get("position", s))
	{
		ReadStringToDoubleVector<double3>(s.data(), s.length(), &pos);
	}
	if (jsonObj.Get("rotation", s))
	{
		ReadStringToVector<float4>(s.data(), s.length(), &rot);
	}
	if (jsonObj.Get("localscale", s))
	{
		ReadStringToVector<float3>(s.data(), s.length(), &scale);
	}
	Vector4 quaternion = rot;
	Matrix4 rotationMatrix = XMMatrixRotationQuaternion(quaternion);
	float3 right, up, forward;
	right = normalize((Vector3&)rotationMatrix[0]);
	up = normalize((Vector3&)rotationMatrix[1]);
	forward = normalize((Vector3&)rotationMatrix[2]);
	std::lock_guard<std::mutex> lck(transMutex);
	randVec.Add(
		{
			up,
			forward,
			right,
			scale,
			{(float)pos.x, (float)pos.y, (float)pos.z},
			isWorldMovable
		},
		&vectorPos
	);

}

ObjectPtr<Transform> Transform::GetTransform(
	const Math::Vector3& position, const Math::Vector4& rotation, const Math::Vector3& localScale,
	bool isWorldMovable)
{
	ObjectPtr<Transform> trans;
	new Transform(trans, position, rotation, localScale, isWorldMovable);
	return trans;
}

Transform::Transform(ObjectPtr<Transform>& targetPtr,
	const Math::Vector3& pos,
	const Math::Vector4& rot,
	const Math::Vector3& scale,
	bool isWorldMovable)
{
	World* world = World::GetInstance();
	if (world != nullptr)
	{
		std::lock_guard<std::mutex> lck(world->mtx);
		targetPtr = ObjectPtr<Transform>::MakePtr(this);
		world->allTransformsPtr.Add(targetPtr, (uint*)&worldIndex);
	}
	else
	{
		worldIndex = -1;
	}
	vengine::string s;
	Vector4 quaternion = rot;
	Matrix4 rotationMatrix = XMMatrixRotationQuaternion(quaternion);
	float3 right, up, forward;
	right = normalize((Vector3&)rotationMatrix[0]);
	up = normalize((Vector3&)rotationMatrix[1]);
	forward = normalize((Vector3&)rotationMatrix[2]);
	std::lock_guard<std::mutex> lck(transMutex);
	
	randVec.Add(
		{
			up,
			forward,
			right,
			scale,
			(float3)pos,
			isWorldMovable
		},
		&vectorPos
	);
}
Transform::Transform(ObjectPtr<Transform>& targetPtr,
	bool isWorldMovable)
{
	World* world = World::GetInstance();
	if (world != nullptr)
	{
		std::lock_guard<std::mutex> lck(world->mtx);
		targetPtr = ObjectPtr<Transform>::MakePtr(this);
		world->allTransformsPtr.Add(targetPtr, (uint*)&worldIndex);
	}
	else
	{
		worldIndex = -1;
	}

	randVec.Add(
		{
			float3(0, 1, 0),
			float3(0, 0, 1),
			float3(1, 0, 0),
			float3(1, 1, 1),
			float3(0, 0, 0),
			isWorldMovable
		},
		&vectorPos
	);
}

ObjectPtr<Transform> Transform::GetTransform(bool isWorldMovable)
{
	ObjectPtr<Transform> objPtr;
	new Transform(objPtr, isWorldMovable);
	return objPtr;
}
ObjectPtr<Transform> Transform::GetTransform(const neb::CJsonObject& path, bool isWorldMovable)
{
	ObjectPtr<Transform> objPtr;
	new Transform(path, objPtr, isWorldMovable);
	return objPtr;
}

void Transform::SetLocalScale(const XMFLOAT3& localScale)
{
	randVec[vectorPos].localScale = localScale;
}

Math::Matrix4 Transform::GetLocalToWorldMatrixCPU() const
{
	Math::Matrix4 target;
	TransformData& data = randVec[vectorPos];
	XMVECTOR vec = XMLoadFloat3(&data.right);
	vec *= data.localScale.x;
	target[0] = vec;
	vec = XMLoadFloat3(&data.up);
	vec *= data.localScale.y;
	target[1] = vec;
	vec = XMLoadFloat3(&data.forward);
	vec *= data.localScale.z;
	target[2] = vec;
	/*target[0].m128_f32[3] = data.position.x;
	target[1].m128_f32[3] = data.position.y;
	target[2].m128_f32[3] = data.position.z;*/
	target[3] = Vector4(data.position, 1);
	return transpose(target);
}
Math::Matrix4 Transform::GetLocalToWorldMatrixGPU() const
{
	Math::Matrix4 target;
	TransformData& data = randVec[vectorPos];
	XMVECTOR vec = XMLoadFloat3(&data.right);
	vec *= data.localScale.x;
	target[0] = vec;
	vec = XMLoadFloat3(&data.up);
	vec *= data.localScale.y;
	target[1] = vec;
	vec = XMLoadFloat3(&data.forward);
	vec *= data.localScale.z;
	target[2] = vec;
	/*target[0].m128_f32[3] = data.position.x;
	target[1].m128_f32[3] = data.position.y;
	target[2].m128_f32[3] = data.position.z;*/
	target[3] = Vector4(data.position, 1);
	return target;
}

Math::Matrix4 Transform::GetWorldToLocalMatrixCPU() const
{
	return inverse(GetLocalToWorldMatrixCPU());
}
Math::Matrix4 Transform::GetWorldToLocalMatrixGPU() const
{
	return inverse(GetLocalToWorldMatrixGPU());
}

void Transform::Dispose()
{
	for (uint i = 0; i < allComponents.Length(); ++i)
	{
		if (allComponents[i])
		{
			allComponents[i].Destroy();
		}
	}
	allComponents.Clear();
}

Transform::~Transform()
{
	if (allComponents.Length() > 0)
		throw "Components Not Disposed!";
	World* world = World::GetInstance();
	if (world)
	{
		if (worldIndex >= 0)
		{
			std::lock_guard<std::mutex> lck(world->mtx);
			world->allTransformsPtr.Remove(worldIndex);
		}
		std::lock_guard<std::mutex> lck(transMutex);
		randVec.Remove(vectorPos);
	}
}

void Transform::DisposeTransform(ObjectPtr<Transform>& trans)
{
	trans->Dispose();
	trans.Destroy();
}