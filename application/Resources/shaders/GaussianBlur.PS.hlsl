#include "PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer BlurParams : register(b0)
{
    float2 texelSize;
    float2 blurDirection;
    float radius;
    float padding[3];
}

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float3 result = float3(0, 0, 0);
    float totalWeight = 0.0;
    
    float2 offset = blurDirection * texelSize;
    int sampleCount = 13; // 固定
    float sigma = radius * 0.4; // シグマ値
    
    // 動的に重みを計算（必ず正規化される）
    for (int i = 0; i < sampleCount; i++)
    {
        float sampleOffset = float(i - sampleCount / 2);
        float2 sampleUV = input.texcoord + offset * sampleOffset;
        
        // ガウシアン重み計算
        float weight = exp(-(sampleOffset * sampleOffset) / (2.0 * sigma * sigma));
        
        // 境界処理
        float3 sampleColor = gTexture.Sample(gSampler, saturate(sampleUV)).rgb;
        
        result += sampleColor * weight;
        totalWeight += weight;
    }
    
    // 重要：必ず正規化する
    result /= totalWeight;
    
    output.color = float4(result, 1.0);
    return output;
}