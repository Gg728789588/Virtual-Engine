#include "Include/Sampler.cginc"
TextureCube _MainTex : register(t0, space0);
cbuffer SkyboxBuffer : register(b0)
{
    float4x4 invVP;
    float4x4 _NonJitterVP;
    float4x4 _LastSkyVP;
};


struct appdata
{
	float3 vertex    : POSITION;
};

struct v2f
{
	float4 position    : SV_POSITION;
    float3 worldView : TEXCOORD0;
    float4 currentProjPos : TEXCOORD1;
    float4 lastProjPos : TEXCOORD2;
};

v2f vert(appdata v)
{
    v2f o;
    o.position = float4(v.vertex.xy, 0, 1);
    float4 worldPos = mul(invVP, float4(v.vertex.xy, 1, 1));
    o.currentProjPos = mul(_NonJitterVP, worldPos);
    o.lastProjPos = mul(_LastSkyVP, worldPos);
    worldPos.xyz /= worldPos.w;
    o.worldView = worldPos.xyz;
    return o;
}

void frag(v2f i, out float4 color : SV_TARGET0,
out float2 mv : SV_TARGET1)
{
  
    float2 lastScreenUV = (i.lastProjPos.xy / i.lastProjPos.w) * float2(0.5, 0.5) + 0.5;
    float2 screenUV = (i.currentProjPos.xy / i.currentProjPos.w) * float2(0.5, 0.5) + 0.5;
    mv = screenUV - lastScreenUV;
    mv.y = -mv.y;
    color = _MainTex.SampleLevel(bilinearClampSampler, i.worldView, 0);
}