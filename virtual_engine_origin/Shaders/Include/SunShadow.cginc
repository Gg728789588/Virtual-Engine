#ifndef _SUN_SHADOW_INCLUDE
#define _SUN_SHADOW_INCLUDE
#include "Montcalo_Library.hlsl"
inline void GetShadowPos(float eyeDistance, float3 worldPos, out uint zAxisUV, out float4 shadowPos, out float softValue, out float shadowOffset)
{
	float4 eyeRange = eyeDistance < _CascadeDistance;
	eyeRange.yzw -= eyeRange.xyz;
	zAxisUV = (uint)(dot(eyeRange, float4(0,1,2,3)) + 0.5);
	shadowPos = mul(_ShadowMatrix[zAxisUV], float4(worldPos, 1));
	shadowPos /= float4(shadowPos.w, -shadowPos.w, shadowPos.w, shadowPos.w);
	softValue = dot(_ShadowSoftValue, eyeRange);
	shadowOffset = dot(eyeRange, _ShadowOffset);
}

inline void GetShadowPos(float eyeDistance, float3 worldPos, out uint zAxisUV, out float4 shadowPos)
{
	float4 eyeRange = eyeDistance < _CascadeDistance;
	eyeRange.yzw -= eyeRange.xyz;
	zAxisUV = (uint)(dot(eyeRange, float4(0,1,2,3)) + 0.5);
	shadowPos = mul(_ShadowMatrix[zAxisUV], float4(worldPos, 1));
	shadowPos /= float4(shadowPos.w, -shadowPos.w, shadowPos.w, shadowPos.w);
}

float GetShadow(float3 worldPos, float eyeDistance, float nol, uint4 texIndices)
{
	const uint SAMPLECOUNT = 16;
    uint zAxisUV;
	float PCSSValue, offst;
	float4 shadowPos;
	GetShadowPos(eyeDistance, worldPos, zAxisUV, shadowPos, PCSSValue, offst);
	float2 shadowUV = shadowPos.xy;
	if(dot(abs(shadowUV) > 1, 1) > 0.5) return 1;
	shadowUV = shadowUV * 0.5 + 0.5;
	float2 seed = shadowUV;
	float dist = shadowPos.z + offst;
    Texture2D<float> _DirShadowMap = _GreyTex[texIndices[zAxisUV]];
	//float atten = _DirShadowMap.SampleCmpLevelZero(linearShadowSampler, shadowUV, dist);
	float atten = 0;
	PCSSValue *= lerp(0, 1, pow(abs(nol), 0.3333333333));
	
	[loop]
	for (uint i = 0; i < SAMPLECOUNT; ++i) {
		seed = MNoise(seed, _RandomSeed);
		float2 shadowOffset = UniformSampleDiskConcentricApprox(seed);
		atten += _DirShadowMap.SampleCmpLevelZero(linearShadowSampler, shadowUV + shadowOffset * PCSSValue, dist);
	}
	atten /= SAMPLECOUNT;
	float fadeDistance = saturate((_CascadeDistance.w - eyeDistance) / (_CascadeDistance.w * 0.05));
	atten = lerp(1, atten, fadeDistance);
	return atten;
}

float GetHardShadow(float3 worldPos, float eyeDistance, uint4 texIndices)
{
	uint zAxisUV;
	float4 shadowPos;
	GetShadowPos(eyeDistance, worldPos, zAxisUV, shadowPos);
	float2 shadowUV = shadowPos.xy;
	if(dot(abs(shadowUV) > 1, 1) > 0.5) return 1;
	shadowUV = shadowUV * 0.5 + 0.5;
	float2 seed = shadowUV;
	float dist = shadowPos.z;
    Texture2D<float> _DirShadowMap = _GreyTex[texIndices[zAxisUV]];
	
	float atten = _DirShadowMap.SampleCmpLevelZero(linearShadowSampler, shadowUV, dist);
	float fadeDistance = saturate((_CascadeDistance.w - eyeDistance) / (_CascadeDistance.w * 0.05));
	atten = lerp(1, atten, fadeDistance);
	return atten;
}


#endif