// G-Buffer Pass Pixel Shader
// ジオメトリパス用ピクセルシェーダー

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPos : TEXCOORD1;
};

// G-Buffer出力
struct GBufferOutput
{
    float4 albedo : SV_TARGET0; // 0番（色）へ出力
    float4 normal : SV_TARGET1; // 1番（法線）へ出力
    float4 material : SV_TARGET2; // 2番（材質）へ出力
    float4 emissive : SV_TARGET3; // 3番（発光）へ出力
};

// マテリアル（C++側のMaterial構造体に合わせる）
cbuffer Material : register(b2)
{
    float4 materialColor;
    int enableLighting;
    float3 pad1;
    float4x4 uvTransform;
    float shininess;
    float reflectivity;
    float2 pad2;
};

// テクスチャ
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// 法線をエンコード（-1,1 -> 0,1）
float3 EncodeNormal(float3 normal)
{
    return normal * 0.5f + 0.5f;
}

GBufferOutput main(PixelShaderInput input)
{
    GBufferOutput output;
    
    // テクスチャカラー
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    
    // アルファテスト
    if (texColor.a < 0.5f)
    {
        discard;
    }
    
    // Albedo + Metallic
    float metallic = reflectivity;
    output.albedo = float4(materialColor.rgb * texColor.rgb, metallic);
    
    // Normal（ワールド空間、エンコード済み）
    float3 normal = normalize(input.normal);
    output.normal = float4(EncodeNormal(normal), 1.0f);
    
    // Material properties
    float roughness = 1.0f - saturate(shininess / 256.0f);
    float ao = 1.0f;
    output.material = float4(roughness, ao, 0.0f, 0.0f);
    
    // Emissive
    output.emissive = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    return output;
}
