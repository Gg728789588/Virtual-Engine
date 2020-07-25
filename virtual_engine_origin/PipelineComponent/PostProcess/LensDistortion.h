#pragma once
#include "../../Common/d3dUtil.h"
#include "../../CJsonObject/CJsonObject.hpp"
#define DEFINE_STR(name) static vengine::string name##_str = #name
#define KEYVALUE_PAIR(n, t) JsonKeyValuePair<n>(t##_str, t)
using namespace neb;

struct LensDistortion
{
	CJsonObject* jsonData;
	LensDistortion(CJsonObject* jsonData) : 
		jsonData(jsonData)
	{

	}
	void GetLensDistortion(
		float4& centerScale,
		float4& amountResult
	) {
		int intensity = 5;
		float centerX = 0;
		float centerY = 0;
		float intensityX = 1;
		float intensityY = 1;
		float scale = 1;

		DEFINE_STR(intensity);
		DEFINE_STR(intensityX);
		DEFINE_STR(intensityY);
		DEFINE_STR(centerX);
		DEFINE_STR(centerY);
		DEFINE_STR(scale);
		GetValuesFromJson(jsonData,
			KEYVALUE_PAIR(int, intensity),
			KEYVALUE_PAIR(float, centerX),
			KEYVALUE_PAIR(float, centerY),
			KEYVALUE_PAIR(float, intensityX),
			KEYVALUE_PAIR(float, intensityY),
			KEYVALUE_PAIR(float, scale));
		float amount = abs(intensity);
		amount = 1.6f * Max<float>(amount, 1);
		float theta = 0.0174532924 * Min<float>(160, amount);
		float sigma = 2.0 * tan(theta * 0.5);
		Vector4 p0(centerX, centerY, Max<float>(intensityX, 1e-4), Max<float>(intensityY, 1e-4));
		Vector4 p1(intensity >= 0 ? theta : 1.0 / theta, sigma, 1.0 / scale, intensity);
		centerScale = p0;
		amountResult = p1;
		

	}
};

struct ChromaticAberration
{
	CJsonObject* jsonObj;
	ChromaticAberration(CJsonObject* jsonObj) : 
		jsonObj(jsonObj)
	{

	}
	bool Run(float& chromaticAberrationInten)
	{
		float intensity = 0.15f;
		int enabled = 0;
		DEFINE_STR(intensity);
		DEFINE_STR(enabled);
		jsonObj->Get(intensity_str, intensity);
		jsonObj->Get(enabled_str, enabled);
		chromaticAberrationInten = intensity * 0.05f;
		return enabled;
	}
};
#undef DEFINE_STR
#undef KEYVALUE_PAIR