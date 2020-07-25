#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/MObject.h"
#include "../LogicComponent/Component.h"
#include <math.h>
#include "../Common/RandomVector.h"
#include "../LogicComponent/Transform.h"
class RenderTexture;
class UploadBuffer;
class World;
struct LightCommand
{
	float4x4 spotVPMatrix;
	float3 direction;
	int shadowmapIndex;
	float3 lightColor;
	uint lightType;
	float3 position;
	float spotAngle;
	float shadowBias;
	float shadowNearPlane;
	float range;
	float spotRadius;
	float spotSmallAngle;
};

namespace neb { class CJsonObject; }
class LocalShadowRunnable;
class Light final : public Component
{
	friend class LocalShadowRunnable;
public:
	enum LightType
	{
		LightType_Point = 0,
		LightType_Spot = 1
	};
private:
	float intensity = 1;
	float3 color = { 1,1,1 };
	LightType lightType = LightType_Point;
	bool isDirty = true;
	float shadowBias = 0.05f;
	float shadowNearPlane = 0.2f;
	float range = 5;
	float angle = 60;
	float smallAngle = 40;
	bool updateNextFrame = true;
	bool updateEveryFrame = true;
	ObjectPtr<RenderTexture> shadowmap;
	static RandomVector<Light*, true> allLights;
	uint listIndex;
	Light(const ObjectPtr<Transform>& trans, ObjectPtr<Component>& ptr);
public:
	inline static ObjectPtr<Light> CreateLight(const ObjectPtr<Transform>& trans)
	{
		ObjectPtr<Component> comp;
		new Light(trans, comp);
		return comp.CastTo<Light>();
	}
	static void GetAllLightings(
		ArrayList<LightCommand>& lightingResults,
		ArrayList<Light*>& lightPtrs,
		Math::Vector4* frustumPlanes,
		const Math::Vector4& frustumMinPoint,
		const Math::Vector4& frustumMaxPoint);
	void GetDataFromJson(ID3D12Device* device, const neb::CJsonObject& cjson);
	virtual void OnEnable() override;
	virtual void OnDisable() override;
	bool IsShadowPerFrameUpdate() const
	{
		return updateEveryFrame;
	}
	void SetShadowPerFrameUpdate(bool value)
	{
		updateEveryFrame = value;
	}
	void UpdateShadowNow()
	{
		updateNextFrame = true;
	}

	float GetIntensity() const { return intensity; }
	void SetIntensity(float intensity) {

		intensity = Max<float>(0, intensity);
		isDirty = true;
		this->intensity = intensity;
	}
	float GetSpotAngle() const { return angle; }
	float GetRange() const { return range; }
	void SetRange(float data) { range = data; }
	void SetSpotAngle(float angle)
	{
		isDirty = true;
		angle = Max<float>(1, angle);
		this->angle = angle;
	}
	float GetSpotSmallAngle() const { return smallAngle; }
	void SetSpotSmallAngle(float value) { smallAngle = value; }
	float3 GetColor() const { return color; }
	void SetColor(float3 value)
	{
		isDirty = true;
		color = Max<Math::Vector3>(value, color);
	}
	bool ShadowEnabled() const { return shadowmap; }
	RenderTexture* GetShadowmap() const
	{
		return shadowmap;
	}
	ObjectPtr<RenderTexture> GetShadowmapObjectPtr() const
	{
		return shadowmap;
	}
	void SetShadowEnabled(bool value, ID3D12Device* device);

	float GetShadowBias() const
	{
		return shadowBias;
	}
	void SetShadowBias(float value)
	{
		isDirty = true;
		value = Max(0.01f, value);
		shadowBias = value;
	}
	float GetShadowNearPlane() const
	{
		return shadowNearPlane;
	}
	void SetShadowNearPlane(float value)
	{
		isDirty = true;
		value = Max(0.1f, value);
		shadowNearPlane = value;
	}
	LightType GetLightType() const
	{
		return lightType;
	}
	void SetLightType(LightType type)
	{
		isDirty = true;
		lightType = type;
	}
	LightCommand GetLightCommand(float radius);
};

