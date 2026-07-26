#include "PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer BrightPassParams : register(b0)
{
    float threshold;
    float intensity;
    float knee;
    float padding;
}

static const float3 LUMA = float3(0.299, 0.587, 0.114);

// 改良されたしきい値関数
float3 ExtractBright(float3 color, float threshold, float knee)
{
    float luminance = dot(color, LUMA);
    
    // ソフトしきい値を使用
    float softThreshold = threshold - knee;
    
    // 滑らかなしきい値適用
    float contribution = 0.0;
    if (luminance > softThreshold)
    {
        if (luminance < threshold)
        {
            float t = (luminance - softThreshold) / (threshold - softThreshold);
            contribution = t * t * (3.0 - 2.0 * t); // スムーズステップ
        }
        else
        {
            contribution = 1.0;
        }
    }
    
    return color * contribution * intensity;
}

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float3 color = gTexture.Sample(gSampler, input.texcoord).rgb;
    float3 brightColor = ExtractBright(color, threshold, knee);
    
    output.color = float4(brightColor, 1.0);
    return output;
}