#include "Include/Sampler.cginc"
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
Texture2D<float4> _MainTex : register(t0);
v2f vert(appdata v)
{
    v2f o;
    o.position = float4(v.vertex.xy, 1, 1);
    o.uv = v.uv;
    return o;
}

float4 frag_point(v2f i) : SV_TARGET
{
    return _MainTex.SampleLevel(pointClampSampler, i.uv, 0);
}

float4 frag_bilinear(v2f i) : SV_TARGET
{
    return _MainTex.SampleLevel(bilinearClampSampler, i.uv, 0);
}

float4 frag_antiColor(v2f i) : SV_TARGET
{
    return 1 - _MainTex.SampleLevel(bilinearClampSampler, i.uv, 0);
}
