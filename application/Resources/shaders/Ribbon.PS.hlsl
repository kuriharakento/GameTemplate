// Ribbon.PS.hlsl
// リボンレンダラー用のピクセルシェーダー
// Premultiplied Alpha対応（強化版）

#include "Ribbon.hlsli"

// マテリアル定数バッファ
cbuffer Material : register(b0)
{
    float4 materialColor;
    int enableLighting;
    int useTextureColor;
    float2 materialPadding;
    float4x4 uvTransform;
}

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // UV変換
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // 頂点アルファを取得
    float vertexAlpha = input.color.a;
    
    // 最終アルファを計算
    float finalAlpha = vertexAlpha * materialColor.a * textureColor.a;
    
    // アルファが非常に低い場合は早期に破棄
    if (finalAlpha < 0.001)
    {
        discard;
    }
    
    // ベースカラーを計算
    float3 baseColor;
    if (useTextureColor)
    {
        // テクスチャカラーも使用
        baseColor = materialColor.rgb * textureColor.rgb * input.color.rgb;
    }
    else
    {
        // 頂点カラーベース（テクスチャのアルファのみ使用）
        baseColor = input.color.rgb * materialColor.rgb;
    }
    
    // ★ Premultiplied Alpha出力
    // RGB = baseColor * finalAlpha（事前乗算）
    // A = finalAlpha
    output.color.rgb = baseColor * finalAlpha;
    output.color.a = finalAlpha;
    
    return output;
}
