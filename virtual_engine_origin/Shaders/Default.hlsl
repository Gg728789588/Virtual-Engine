#include "Include/Sampler.cginc"
#define GBUFFER_SHADER
Texture2D<float4> _MainTex[] : register(t0, space1);
Texture2D<float> _GreyTex[] : register(t0, space2);
Texture2D<uint> _IntegerTex[] : register(t0, space3);
TextureCube<float4> _Cubemap[] : register(t0, space4);
TextureCube<float> _GreyCubemap[] : register(t0, space5);
Texture3D<uint> _IntegerTex3D[] : register(t0, space6);

struct PerObjectBuffer
{
	float3 worldPos;
};
cbuffer Per_Camera_Buffer : register(b1)
{
	float4x4 _WorldToCamera;
	float4x4 _InverseWorldToCamera;
	float4x4 _Proj;
	float4x4 _InvProj;
	float4x4 _VP;
	float4x4 _InvVP;
	float4x4 _NonJitterVP;
	float4x4 _NonJitterInverseVP;
	float4x4 _LastVP;
	float4x4 _InverseLastVP;
	float4x4 _FlipProj;
	float4x4 _FlipInvProj;
	float4x4 _FlipVP;
	float4x4 _FlipInvVP;
	float4x4 _FlipNonJitterVP;
	float4x4 _FlipNonJitterInverseVP;
	float4x4 _FlipLastVP;
	float4x4 _FlipInverseLastVP;
	float4 _ZBufferParams;
	float4 _RandomSeed;
	float3 worldSpaceCameraPos;
	float _NearZ;
	float _FarZ;
};

cbuffer LightCullCBuffer : register(b2)
{
	float4 _CameraNearPos;
	float4 _CameraFarPos;
	float3 _CameraForward;
	uint _LightCount;
	float3 _SunColor;
	uint _SunEnabled;
	float3 _SunDir;
	uint _SunShadowEnabled;
	uint4 _ShadowmapIndices;
	float4 _CascadeDistance;
	float4x4 _ShadowMatrix[4];
	float4 _ShadowSoftValue;
	float4 _ShadowOffset;
	uint _ReflectionProbeCount;
};

#include "Include/Lighting.cginc"
#include "Include/Random.cginc"

struct SurfaceOutput
{
	float3 albedo;
	float3 specular;
	float smoothness;
	float3 emission;
	float3 normal;
	float occlusion;
};

cbuffer TextureIndices : register(b3)
{
	uint _SkyboxTex;
	uint _PreintTexture;
};

cbuffer ProjectionShadowParams : register(b4)
{
	float4x4 _ShadowmapVP;
	float4 _LightPos;
};

StructuredBuffer<LightCommand> _AllLight : register(t0, space0);
StructuredBuffer<uint> _LightIndexBuffer : register(t1, space0);
StructuredBuffer<PerObjectBuffer> _PerObjectData : register(t2, space0);
inline float LinearEyeDepth( float z )
{
	return 1.0 / (_ZBufferParams.z * z + _ZBufferParams.w);
}
#include "Include/SunShadow.cginc"
struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 uv    : TEXCOORD;
	float4 tangent : TANGENT;
	float2 uv2     : TEXCOORD1;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float4 tangent : TANGENT;
	float4 uv      : TEXCOORD;
	float4 lastProjPos : TEXCOORD1;
	float4 currentProjPos : TEXCOORD2;
};

inline float GetMipLevel(float2 uv)
{
	uv *= 8192;
	float2 dx = ddx(uv);
	float2 dy = ddy(uv);
	float d = max(dot(dx, dx), dot(dy, dy));
	return clamp(0.5 * log2(d) - 0.5, 0, 20);
}

inline float Linear01Depth( float z)
{
	return 1.0 / (_ZBufferParams.x * z + _ZBufferParams.y);
}

SurfaceOutput Surface(float2 uv)
{
	SurfaceOutput o;
	
	o.albedo = 1;

	o.specular = 0.5;
	o.occlusion = 1;//
	o.smoothness = 0.8;
	o.emission = 0;
	o.normal = float3(0, 0, 1);
	return o;
}

VertexOut VS(VertexIn vin, uint instanceID : SV_INSTANCEID)
{
	VertexOut vout = (VertexOut)0.0f;
	PerObjectBuffer objBuffer = _PerObjectData[instanceID];
	// Transform to world space.
	float4 posW = float4(objBuffer.worldPos + vin.PosL, 1);
	vout.PosW = posW.xyz;

	// Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
	vout.NormalW =  vin.NormalL;

	// Transform to homogeneous clip space.
	vout.PosH = mul(_VP, posW);
	vout.currentProjPos = mul(_FlipNonJitterVP, posW);
	vout.lastProjPos = mul(_FlipLastVP, posW);
	vout.uv.xy = float4(vin.uv.xy, vin.uv2.xy);

	vout.tangent.xyz = vin.tangent.xyz;
	vout.tangent.w = vin.tangent.w;
	return vout;
}

void PS(VertexOut i, 
out float4 albedo : SV_TARGET0,
out float4 specular : SV_TARGET1,
out float4 normalTex : SV_TARGET2,
out float2 motionVectorTex : SV_TARGET3,
out float4 emissionRT : SV_TARGET4)

{
	SurfaceOutput surf = Surface(i.uv.xy);

	float2 lastScreenUV = (i.lastProjPos.xy / i.lastProjPos.w) * float2(0.5, 0.5) + 0.5;
	float2 screenUV = (i.currentProjPos.xy / i.currentProjPos.w) * float2(0.5, 0.5) + 0.5;
	motionVectorTex = screenUV - lastScreenUV;
	screenUV.y = 1 - screenUV.y;
	
	float3 viewDir = normalize(worldSpaceCameraPos - i.PosW.xyz);
	i.NormalW = normalize(i.NormalW);
	float roughness = 1 - surf.smoothness;
	float linearEyeDepth = LinearEyeDepth(i.PosH.z);
	BSDFContext context = (BSDFContext)0;
	float3 sunColor = clamp(CalculateSunLight_NoShadow(
	i.NormalW,
	viewDir,
	-_SunDir,
	_SunColor,
	surf.albedo,
	surf.specular,
	roughness,
	context
	), 0, 32768);
	float2 preintAB = _MainTex[_PreintTexture].SampleLevel(bilinearClampSampler, float2(roughness, context.NoV), 0).rg;
	float3 EnergyCompensation;
	float3 preint = (PreintegratedDGF_LUT(preintAB, EnergyCompensation,surf.specular));
	preint *= EnergyCompensation;
	float sunShadow = (GetShadow(i.PosW, linearEyeDepth, dot(i.NormalW, -_SunDir), _ShadowmapIndices));
	
	emissionRT = float4(sunColor * sunShadow + surf.emission, 1);
	emissionRT.xyz += (clamp(CalculateLocalLight(
	screenUV,
	i.PosW.xyz,
	linearEyeDepth,
	i.NormalW,
	viewDir,
	_CameraNearPos.w,
	_CameraFarPos.w,
	surf.albedo,
	surf.specular,
	roughness,
	_LightIndexBuffer,
	_AllLight
	), 0, 32768));
	emissionRT = KillNaN(emissionRT);
	albedo = float4(surf.albedo,surf.occlusion);
	specular = float4(preint,surf.smoothness);
	normalTex = float4(i.NormalW * 0.5 + 0.5, 1);
}

float4 VS_Depth(float3 position : POSITION, uint instanceID : SV_INSTANCEID) : SV_POSITION
{
	PerObjectBuffer objBuffer = _PerObjectData[instanceID];
	float4 posW = float4(objBuffer.worldPos + position, 1);
	return mul(_VP, posW);
}

float4 VS_Shadowmap(float3 position : POSITION, uint instanceID : SV_INSTANCEID) : SV_POSITION
{
	PerObjectBuffer objBuffer = _PerObjectData[instanceID];
	float4 posW = float4(objBuffer.worldPos + position, 1);
	return mul(_ShadowmapVP, posW);
}
struct v2f_pointLight
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
};

v2f_pointLight VS_PointLightShdowmap(float3 position : POSITION, uint instanceID : SV_INSTANCEID)
{
	PerObjectBuffer objBuffer = _PerObjectData[instanceID];
	v2f_pointLight o;
	float4 posW = float4(objBuffer.worldPos + position, 1);
	o.position = mul(_ShadowmapVP, posW);
	o.worldPos = posW.xyz;
	return o;
}

float PS_PointLightShadowmap(v2f_pointLight i) : SV_TARGET
{
	float dist = distance(_LightPos.xyz, i.worldPos);
	return dist / _LightPos.w;
}