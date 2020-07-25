#include "Include/Sampler.cginc"
struct appdata
{
	float3 vertex    : POSITION;
};

struct v2f
{
	float4 position    : SV_POSITION;
    float4 proj : TEXCOORD;
};
//TODO Struct
TextureCube<float4> _Cubemap[] : register(t0, space1);
Texture2D<float> _CameraDepthTexture : register(t0, space2);
Texture2D<float4> _CameraGBuffer0 : register(t1, space2);
Texture2D<float4> _CameraGBuffer1 : register(t2, space2);
Texture2D<float4> _CameraGBuffer2 : register(t3, space2);
Texture2D<float4> _MainTex : register(t4, space2);

#include "Include/Reflection.cginc"
cbuffer TextureIndices : register(b0)
{
	uint _SkyboxTex;
	uint _PreintTexture;
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

cbuffer ReflectionProbeData : register(b2)
{
    ReflectionData _ReflData;
}

v2f vert(appdata v)
{
    v2f o;
    float4 worldPos = float4(v.vertex * _ReflData.maxExtent * 2 + _ReflData.position, 1);
    o.position = mul(_VP, worldPos);
    o.proj = o.position;
    return o;
}
float4 frag_rp(v2f i) : SV_TARGET
{
    float depth = _CameraDepthTexture[i.position.xy];
    float4 worldPos = mul(_InvVP, float4(i.proj.xy / i.proj.w, depth, 1));
    worldPos /= worldPos.w;
    if(!IsCubemapInRange(_ReflData, worldPos.xyz))
    {
        discard;
        return 0;
    }
    float4 gbuffer1 = _CameraGBuffer1[i.position.xy];
    float4 gbuffer2 = _CameraGBuffer2[i.position.xy];
    float3 viewDir = normalize(worldPos.xyz - worldSpaceCameraPos);
    return CalculateReflection(_ReflData, worldPos.xyz, viewDir, gbuffer2.xyz * 2 - 1,  gbuffer1.xyz, gbuffer1.w);
}

void frag_rp_gi(v2f i, out float4 specular : SV_TARGET0, out float4 diffuse : SV_TARGET1)
{
    float depth = _CameraDepthTexture[i.position.xy];
    float4 worldPos = mul(_InvVP, float4(i.proj.xy / i.proj.w, depth, 1));
    worldPos /= worldPos.w;
    if(!IsCubemapInRange(_ReflData, worldPos.xyz))
    {
        discard;
        return;
    }
    float4 gbuffer0 = _CameraGBuffer0[i.position.xy];
    float4 gbuffer1 = _CameraGBuffer1[i.position.xy];
    float4 gbuffer2 = _CameraGBuffer2[i.position.xy];
    float3 viewDir = normalize(worldPos.xyz - worldSpaceCameraPos);
    CalculateReflection_DiffuseGI(_ReflData, worldPos.xyz, viewDir, gbuffer0.xyz, gbuffer2.xyz * 2 - 1,  gbuffer1.xyz, gbuffer1.w, diffuse, specular);
    diffuse *= gbuffer2.w;
}

struct appdata_add
{
	float3 vertex    : POSITION;
    float2 uv : TEXCOORD;
};

struct v2f_add
{
	float4 position    : SV_POSITION;
    float2 uv : TEXCOORD;
};

v2f_add vert_add(appdata_add v)
{
    v2f_add o;
    o.position = float4(v.vertex.xy, 0, 1);
    o.uv = v.uv;
    return o;
}

float4 frag_add(v2f_add i) : SV_TARGET
{
    return _MainTex.SampleLevel(pointClampSampler, i.uv, 0);
}

v2f vert_skybox(appdata v)
{
    v2f o;
    o.position = float4(v.vertex.xy, 0, 1);
    o.proj = o.position;
    return o;
}

float3 frag_skybox(v2f i) : SV_TARGET
{
    float depth = _CameraDepthTexture[i.position.xy];
    float4 worldPos = mul(_InvVP, float4(i.proj.xy / i.proj.w, depth, 1));
    worldPos /= worldPos.w;
    float4 gbuffer1 = _CameraGBuffer1[i.position.xy];
    float4 gbuffer2 = _CameraGBuffer2[i.position.xy];
    float3 viewDir = normalize(worldPos.xyz - worldSpaceCameraPos);
    /*
    CalculateSkyboxReflection(
    float3 viewDir,
    float3 worldNormal,
    float3 specular,
    float smoothness,
    TextureCube<float4> cubemap)
    */
    return CalculateSkyboxReflection(viewDir, gbuffer2.xyz * 2 - 1, gbuffer1.xyz, gbuffer1.w, _Cubemap[_SkyboxTex]);
}

void frag_skybox_gi(v2f i, out float3 specular : SV_TARGET0, out float3 diffuse : SV_TARGET1)
{
    float depth = _CameraDepthTexture[i.position.xy];
    float4 worldPos = mul(_InvVP, float4(i.proj.xy / i.proj.w, depth, 1));
    worldPos /= worldPos.w;
    float4 gbuffer0 = _CameraGBuffer0[i.position.xy];
    float4 gbuffer1 = _CameraGBuffer1[i.position.xy];
    float4 gbuffer2 = _CameraGBuffer2[i.position.xy];
    float3 viewDir = normalize(worldPos.xyz - worldSpaceCameraPos);
    /*
    CalculateSkyboxReflection(
    float3 viewDir,
    float3 worldNormal,
    float3 specular,
    float smoothness,
    TextureCube<float4> cubemap)
    */
    CalculateSkyboxReflection_Diffuse(viewDir, gbuffer2.xyz * 2 - 1, gbuffer0.xyz, gbuffer1.xyz, gbuffer1.w, _Cubemap[_SkyboxTex], diffuse, specular);
    diffuse *= gbuffer2.w;
}