#include "Light.h"
#include "../LogicComponent/Transform.h"
#include "UploadBuffer.h"
#include "../Common/MetaLib.h"
#include "../Singleton/MathLib.h"
#include "../LogicComponent/World.h"
#include "RenderTexture.h"
#include "../CJsonObject/CJsonObject.hpp"
#include "../Common/ArrayList.h"
using namespace DirectX;
using namespace Math;
RandomVector<Light*, true> Light::allLights(50);
Light::Light(const ObjectPtr<Transform>& trans, ObjectPtr<Component>& ptr) :
	Component(trans, ptr)
{


}

void Light::GetDataFromJson(ID3D12Device* device, const neb::CJsonObject& cjson)
{
#define DEFINE_STR(x) static vengine::string x##_str = #x
	DEFINE_STR(lightType);
	DEFINE_STR(intensity);
	DEFINE_STR(shadowBias);
	DEFINE_STR(shadowNearPlane);
	DEFINE_STR(range);
	DEFINE_STR(angle);
	DEFINE_STR(color);
	DEFINE_STR(smallAngle);
	DEFINE_STR(useShadow);
	DEFINE_STR(shadowCache);
	cjson.Get(lightType_str, *(int*)&lightType);
	cjson.Get(intensity_str, intensity);
	cjson.Get(shadowBias_str, shadowBias);
	cjson.Get(shadowNearPlane_str, shadowNearPlane);
	cjson.Get(range_str, range);
	cjson.Get(angle_str, angle);
	cjson.Get(smallAngle_str, smallAngle);
	smallAngle = Min(angle, smallAngle);
	vengine::string value;
	if (cjson.Get(color_str, value))
	{
		ReadStringToVector<XMFLOAT3>(value.data(), value.size(), &color);
	}
	int useShadow = 0;
	int shadowCache = 0;
	cjson.Get(useShadow_str, useShadow);
	cjson.Get(shadowCache_str, shadowCache);
#undef DEFINE_STR
	SetShadowEnabled(useShadow, device);
	updateEveryFrame = (shadowCache == 0);
	updateNextFrame = true;
}
void Light::OnEnable()
{
	allLights.Add(this, &listIndex);
}

void Light::SetShadowEnabled(bool value, ID3D12Device* device) {
	isDirty = true;
	if (value != shadowmap)
	{
		if (value)
		{
			switch (lightType)
			{
			case LightType::LightType_Point:
				shadowmap = ObjectPtr<RenderTexture>::NewObject(
						device, 1024, 1024, RenderTextureFormat::GetColorFormat(DXGI_FORMAT_R16_UNORM), TextureDimension::Cubemap, 6, 0, RenderTextureState::Generic_Read,
						nullptr, 0, 1
				);
				break;
			case LightType::LightType_Spot:
				shadowmap = ObjectPtr<RenderTexture>::NewObject(
						device, 2048, 2048, RenderTextureFormat::GetDepthFormat(RenderTextureDepthSettings_Depth16), TextureDimension::Tex2D, 1, 0, RenderTextureState::Generic_Read
				);
				break;
			}
		}
		else
		{
			shadowmap.Destroy();
			shadowmap = nullptr;
		}
	}
}
void Light::OnDisable()
{
	allLights.Remove(listIndex);
}

LightCommand Light::GetLightCommand(float radius)
{
	return
	{
		float4x4(),
		GetTransform()->GetForward(),
		shadowmap ? (int)shadowmap->GetGlobalDescIndex() : -1,
		{color.x * intensity, color.y * intensity, color.z * intensity},
		(UINT)lightType,
		GetTransform()->GetPosition(),
		(float)(angle * Deg2Rad * 0.5),
		shadowBias,
		shadowNearPlane,
		range,
		radius,
		(float)(smallAngle * Deg2Rad * 0.5)
	};
}
void Light::GetAllLightings(
	ArrayList<LightCommand>& lightingResults,
	ArrayList<Light*>& lightPtrs,
	Vector4* frustumPlanes,
	const Vector4& frustumMinPoint,
	const Vector4& frustumMaxPoint)
{
	lightingResults.clear();
	lightPtrs.clear();
	for (uint i = 0; i < allLights.Length(); ++i)
	{
		auto ite = allLights[i];
		Vector3 position = ite->GetTransform()->GetPosition();
		Vector3 forward = ite->GetTransform()->GetForward();
		Vector3 range = { ite->range, ite->range, ite->range };
		Vector3 minPoint = position - range;
		Vector3 maxPoint = position + range;
		auto les = ((Vector3)frustumMinPoint < maxPoint) &&
			(minPoint < (Vector3)frustumMaxPoint);

		if (les.x && les.y && les.z) {
			switch (ite->lightType)
			{
			case LightType_Spot:
			{
				Cone spotCone(position, ite->range, ite->GetTransform()->GetForward(), ite->angle);
				auto func = [&](UINT i)->bool
				{
					return MathLib::ConeIntersect(spotCone, frustumPlanes[i]);
				};
				if (InnerLoopEarlyBreak<decltype(func), 6>(func))
				{
					lightingResults.push_back(ite->GetLightCommand(spotCone.radius));
					lightPtrs.push_back(ite);
				}
			}
			break;
			case LightType_Point:
			{
				auto func = [&](UINT i)->bool
				{
					return MathLib::GetPointDistanceToPlane((frustumPlanes[i]), (position)) < ite->range;
				};
				if (InnerLoopEarlyBreak<decltype(func), 6>(func))
				{
					lightingResults.push_back(ite->GetLightCommand(0));
					lightPtrs.push_back(ite);
				}
			}
			break;
			}
		}
	}
}