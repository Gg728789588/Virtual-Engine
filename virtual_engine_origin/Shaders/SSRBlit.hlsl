#include "Include/Sampler.cginc"
Texture2D<float4> _MainTex : register(t0);
Texture2D<float4> SRV_GBufferRoughness : register(t1);
struct appdata
{
	float3 vertex    : POSITION;
    float2 uv : TEXCOORD;
};

struct v2f
{
	float4 position    : SV_POSITION;
    float2 uv : TEXCOORD;
};

v2f vert(appdata v)
{
    v2f o;
    o.position = float4(v.vertex.xy, 1, 1);
    o.uv = v.uv;
    return o;
}

float4 frag(v2f i) : SV_TARGET
{
    float4 data = _MainTex.SampleLevel(pointClampSampler, i.uv, 0);
    float3 specular = SRV_GBufferRoughness.SampleLevel(pointClampSampler, i.uv, 0);
    data.xyz *= specular;
    return data;
}
