#include "Include/Sampler.cginc"

StructuredBuffer<int3> _DescriptorIndexBuffer : register(t0, space0);
Texture2D<float4> _MainTex[100] : register(t0, space1);
Texture2D<uint4> _IndexMap[100] : register(t0, space2);
Texture2D<float2> _RGTex[100] : register(t0, space3);

cbuffer Params : register(b0)
{
    float2 _IndexMapSize;
    float2 _SplatScale;
    float2 _SplatOffset;
    float2 _VTOffset;
    float _VTSize;
    uint _RandomTileSize;
    uint _SplatMapIndex;
    uint _IndexMapIndex;
    uint _DescriptorCount;
    uint _NoiseTexIndex;
    uint _RandomTileIndex;
};

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

float2 GetUVOffset(uint2 chunkUV, float2 localUV, Texture2D<float2> randomTex, Texture2D<float4> scaleOffsetTexture, float2 scaleOffsetTexelSize, float sampleScale)
{
    float2 sampleUV = chunkUV * scaleOffsetTexelSize.xy + (localUV + randomTex.SampleLevel(bilinearWrapSampler, localUV, 0)) * scaleOffsetTexelSize.xy;
    sampleUV *= sampleScale;
    float4 scaleOffset = scaleOffsetTexture.SampleLevel(bilinearWrapSampler, sampleUV, 0);
    return localUV * scaleOffset.xy + scaleOffset.zw;
}

void frag(v2f i,
out float4 albedoSmo : SV_TARGET0,
out float2 normal : SV_TARGET1,
out float2 metaOcc : SV_TARGET2)
{
    float3 albedo = 0;
    float3 smo = 0;
    float2 splatUV = saturate(i.uv * _SplatScale + _SplatOffset);
    float2 vtUV = i.uv * _VTSize;
    
    float mipLevel = max(0, log2(_VTSize));
    float2 sampleUV = (vtUV + _VTOffset) * 0.5;
    /*
    uint2 randTileUV = (uint2)((_VTOffset + vtUV + (_RGTex[_NoiseTexIndex].SampleLevel(bilinearWrapSampler, vtUV, 0) * 2 - 1) * 0.2) * 4) % _RandomTileSize;
    float4 randTile = _MainTex[_RandomTileIndex].Load(uint3(randTileUV, 0));
    randTile.xy = randTile.xy * 2 - 1;
   
    sampleUV = sampleUV * randTile.xy + randTile.zw; */
    float4 splatValue = _MainTex[_SplatMapIndex].SampleLevel(bilinearWrapSampler, splatUV, 0);
    uint4 indexValue = _IndexMap[_IndexMapIndex].Load(uint3((uint2)(_IndexMapSize * splatUV), 0));
    float sumSplat = 0;
    albedo = 0;
    smo = 0;
    normal = 0;
    for(uint ite = 0; ite < 4; ++ite)
    {
        int3 idx = _DescriptorIndexBuffer[indexValue[ite]];
        if(idx.x >= 0)
            albedo += _MainTex[idx.x].SampleLevel(bilinearWrapSampler, sampleUV, mipLevel) * splatValue[ite];
        if(idx.y >= 0)
            normal += _MainTex[idx.y].SampleLevel(bilinearWrapSampler, sampleUV, mipLevel) * splatValue[ite];
        if(idx.z >= 0)
            smo += _MainTex[idx.z].SampleLevel(bilinearWrapSampler, sampleUV, mipLevel) * splatValue[ite];
        sumSplat += splatValue[ite];
    }
    albedo /= sumSplat;
    albedo = pow(albedo, 2.2);
    normal /= sumSplat;
    smo /= sumSplat;
    albedoSmo = float4(albedo, smo.x);
    metaOcc = smo.yz;
}
