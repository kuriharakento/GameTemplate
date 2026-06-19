#include "PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0); // メインシーン
Texture2D<float4> gBloomTexture : register(t1); // ブルーム
SamplerState gSampler : register(s0);

cbuffer PostEffectParams : register(b0)
{
    // Grayscale
    float grayscaleIntensity;
    int grayscaleEnabled;
    float2 pad0;

    // Vignette
    int vignetteEnabled;
    float vignetteIntensity;
    float vignetteRadius;
    float vignetteSoftness;
    float3 vignetteColor;
    float pad1;

    // Noise
    int noiseEnabled;
    float noiseIntensity;
    float noiseTime;
    float grainSize;
    float luminanceAffect;
    float3 pad2;

    // CRT
    int crtEnabled;
    int scanlineEnabled;
    float scanlineIntensity;
    float scanlineCount;
    int distortionEnabled;
    float distortionStrength;
    int chromAberrationEnabled;
    float chromAberrationOffset;
    float4 pad3;

    // Bloom
    int bloomEnabled;
    float bloomIntensity;
    float bloomThreshold;
    float bloomRadius;
    float3 pad4;
    float2 invScreenSize;
    float bloomThresholdKnee;
    float bloomMix;
};

// 最適化されたランダム関数
float fastRandom(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453 + noiseTime * 0.1);
}

// 歪み適用（常に計算、係数で制御）
float2 applyDistortion(float2 uv, float strength)
{
    float2 offset = uv - 0.5;
    return uv + offset * strength * dot(offset, offset);
}

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    float2 uv = input.texcoord;
    
    // フラグを正規化（0.0 or 1.0）しておくのはそのまま
    float crtFactor = saturate((float) crtEnabled);
    
    // 変数を宣言（初期値はベースカラー取得までやる）
    float4 baseColor = gTexture.Sample(gSampler, uv);
    float3 color = baseColor.rgb;
    
    // 歪み（重いので条件付き）
    if (crtEnabled != 0 && distortionEnabled != 0)
    {
        float2 distortedUV = applyDistortion(uv, distortionStrength);
        uv = lerp(uv, distortedUV, 1.0);
        baseColor = gTexture.Sample(gSampler, uv);
        color = baseColor.rgb;
    }
    
    // 色収差（条件付き）
    if (crtEnabled != 0 && chromAberrationEnabled != 0)
    {
        float2 chromOffset = chromAberrationOffset * 0.001;
        float r = gTexture.Sample(gSampler, uv + chromOffset).r;
        float b = gTexture.Sample(gSampler, uv - chromOffset).b;
        float3 chromColor = float3(r, baseColor.g, b);
        color = lerp(color, chromColor, 1.0);
    }
    
    // 走査線（条件付き）
    if (crtEnabled != 0 && scanlineEnabled != 0)
    {
        float scanlinePattern = 1.0 - scanlineIntensity * 0.5 * (1.0 + sin(uv.y * scanlineCount * 6.283185));
        color *= scanlinePattern;
    }
    
    // ノイズ（条件付き）
    if (noiseEnabled != 0)
    {
        float2 grainUV = uv * grainSize + noiseTime;
        float noiseValue = fastRandom(grainUV);
        float luminance = dot(color, float3(0.299, 0.587, 0.114));
        float luminanceFactor = lerp(1.0, luminance, luminanceAffect);
        float finalNoise = (noiseValue - 0.5) * noiseIntensity * luminanceFactor;
        color += finalNoise;
    }
    
    // ビネット（条件付き）
    if (vignetteEnabled != 0)
    {
        float2 vignetteOffset = uv - 0.5;
        float dist = length(vignetteOffset);
        float vignetteEdge = vignetteRadius - vignetteSoftness;
        float vignetteStrength = saturate((dist - vignetteEdge) / vignetteSoftness);
        float3 vignetteResult = lerp(color, vignetteColor, vignetteStrength * vignetteIntensity);
        color = lerp(color, vignetteResult, 1.0);
    }
    
    // グレースケール（条件付き）
    if (grayscaleEnabled != 0)
    {
        float finalLuminance = dot(color, float3(0.2126, 0.7152, 0.0722));
        float3 grayscaleResult = lerp(color, finalLuminance.xxx, grayscaleIntensity);
        color = lerp(color, grayscaleResult, 1.0);
    }
    
    // Bloom処理部分
    if (bloomEnabled != 0)
    {
        float3 bloom = gBloomTexture.Sample(gSampler, uv).rgb;
        color += bloom * bloomIntensity;
    }
    
    PixelShaderOutput output;
    output.color = float4(color, baseColor.a);
    return output;
}