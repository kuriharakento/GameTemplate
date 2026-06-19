#include "Object3d.hlsli"

// マテリアル
struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
    float shininess;
    float reflectivity; // 反射率
    float2 pad2;
};

// ディレクショナルライト
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

// カメラ
struct Camera
{
    float3 worldPos;
};

// ポイントライト
struct GPUPointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
};

// スポットライト
struct GPUSpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
    float cosFalloffStart;
    int shadowEnabled;
    float4 padding;
    row_major float4x4 shadowViewProj;
};

struct LightCounts
{
    uint gPointLightCount;
    uint gSpotLightCount;
};

// シャドウ行列データ（単一シャドウマップ用）
struct ShadowData
{
    float4x4 lightViewProjection;
    int enableShadow;
    float3 padding;
};

// カスケードシャドウデータ
struct CascadeShadowData
{
    float4x4 lightViewProjections[4];
    float4 cascadeSplits; // x,y,z,w = cascade 0,1,2,3
    int enableShadow;
    float3 padding;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
StructuredBuffer<GPUPointLight> gPointLights : register(t3);
StructuredBuffer<GPUSpotLight> gSpotLights : register(t4);
ConstantBuffer<LightCounts> gLightCounts : register(b5);
ConstantBuffer<ShadowData> gShadowData : register(b6);
ConstantBuffer<CascadeShadowData> gCascadeShadowData : register(b7);

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2D<float> gShadowMap : register(t5);
// カスケードシャドウマップ（個別テクスチャ）
Texture2D<float> gCascadeShadowMap0 : register(t6);
Texture2D<float> gCascadeShadowMap1 : register(t7);
Texture2D<float> gCascadeShadowMap2 : register(t8);
Texture2D<float> gCascadeShadowMap3 : register(t9);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

// シャドウマップ設定
static const float SHADOW_BIAS = 0.0005f;
static const float SHADOW_MAP_SIZE = 2048.0f;
static const int PCF_SAMPLES = 1; // -1 to +1 = 3x3 samples

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 効率化された照明計算関数
float3 CalculateHalfLambert(float3 normal, float3 lightDir)
{
    float NdotL = dot(normal, lightDir);
    NdotL = NdotL * 0.5f + 0.5f;
    return pow(NdotL, 2.0f);
}

float3 CalculateSpecular(float3 normal, float3 lightDir, float3 toEye, float3 lightColor, float intensity, float shininess)
{
    float3 halfVector = normalize(lightDir + toEye);
    float NdotH = saturate(dot(normal, halfVector));
    float specularPow = pow(NdotH, shininess);
    return lightColor * intensity * specularPow;
}

// シャドウ計算（カスケードシャドウマップ使用）
// シャドウ計算（カスケードシャドウマップ使用）
float CalculateCascadeShadow(float3 worldPos, float viewDepth)
{
    // カスケードシャドウが無効の場合は影なし
    if (gCascadeShadowData.enableShadow == 0)
    {
        return 1.0f;
    }
    
    // ビュー深度に基づいてカスケードを選択
    int cascadeIndex = 3; // デフォルトは最遠距離カスケード
    float4 splits = gCascadeShadowData.cascadeSplits;
    
    if (viewDepth < splits.x)
        cascadeIndex = 0;
    else if (viewDepth < splits.y)
        cascadeIndex = 1;
    else if (viewDepth < splits.z)
        cascadeIndex = 2;
    else
        cascadeIndex = 3;
    
    // 選択したカスケードのビュー・プロジェクション行列で変換
    float4 lightSpacePos = mul(float4(worldPos, 1.0f), gCascadeShadowData.lightViewProjections[cascadeIndex]);
    
    // パースペクティブディバイド
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    
    // UV座標に変換 [-1,1] -> [0,1]
    float2 shadowUV;
    shadowUV.x = projCoords.x * 0.5f + 0.5f;
    shadowUV.y = -projCoords.y * 0.5f + 0.5f;
    
    // UV座標ベースの範囲外チェック（より緩和）
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f ||
        shadowUV.y < 0.0f || shadowUV.y > 1.0f)
    {
        return 1.0f;
    }
    
    float currentDepth = projCoords.z;

    
    // PCF (Percentage Closer Filtering)
    float shadow = 0.0f;
    float texelSize = 1.0f / SHADOW_MAP_SIZE;
    int sampleCount = 0;
    
    // カスケードに応じたサンプリング（個別テクスチャを使用）
    [unroll]
    for (int x = -PCF_SAMPLES; x <= PCF_SAMPLES; x++)
    {
        [unroll]
        for (int y = -PCF_SAMPLES; y <= PCF_SAMPLES; y++)
        {
            float2 offset = float2(x, y) * texelSize;
            float sampledDepth = 0.0f;
            
            // カスケードインデックスに応じてテクスチャを選択
            if (cascadeIndex == 0)
            {
                sampledDepth = gCascadeShadowMap0.SampleCmpLevelZero(gShadowSampler, shadowUV + offset, currentDepth - SHADOW_BIAS);
            }
            else if (cascadeIndex == 1)
            {
                sampledDepth = gCascadeShadowMap1.SampleCmpLevelZero(gShadowSampler, shadowUV + offset, currentDepth - SHADOW_BIAS);
            }
            else if (cascadeIndex == 2)
            {
                sampledDepth = gCascadeShadowMap2.SampleCmpLevelZero(gShadowSampler, shadowUV + offset, currentDepth - SHADOW_BIAS);
            }
            else
            {
                sampledDepth = gCascadeShadowMap3.SampleCmpLevelZero(gShadowSampler, shadowUV + offset, currentDepth - SHADOW_BIAS);
            }
            
            shadow += sampledDepth;
            sampleCount++;
        }
    }
    
    shadow /= (float)sampleCount;
    
    return shadow;
}


// 旧シャドウ計算（後方互換用）
// 旧シャドウ計算（後方互換用）
float CalculateShadow(float3 worldPos)
{
    // ワールド座標をライト空間に変換
    float4 lightSpacePos = mul(float4(worldPos, 1.0f), gShadowData.lightViewProjection);
    
    // パースペクティブディバイド
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    
    // 範囲外チェック（シャドウマップの外は影なし）
    if (projCoords.x < -1.0f || projCoords.x > 1.0f ||
        projCoords.y < -1.0f || projCoords.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f;
    }
    
    // UV座標に変換 [-1,1] -> [0,1]
    float2 shadowUV;
    shadowUV.x = projCoords.x * 0.5f + 0.5f;
    shadowUV.y = -projCoords.y * 0.5f + 0.5f; // Y軸反転
    
    float currentDepth = projCoords.z;
    
    // PCF (Percentage Closer Filtering)
    float shadow = 0.0f;
    float texelSize = 1.0f / SHADOW_MAP_SIZE;
    int sampleCount = 0;
    
    [unroll]
    for (int x = -PCF_SAMPLES; x <= PCF_SAMPLES; x++)
    {
        [unroll]
        for (int y = -PCF_SAMPLES; y <= PCF_SAMPLES; y++)
        {
            float2 offset = float2(x, y) * texelSize;
            shadow += gShadowMap.SampleCmpLevelZero(
                gShadowSampler,
                shadowUV + offset,
                currentDepth - SHADOW_BIAS
            );
            sampleCount++;
        }
    }
    
    shadow /= (float)sampleCount;
    
    return shadow;
}


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // テクスチャUVとカラーの取得（early out用に先に計算）
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // アルファテスト（early out）
    if (textureColor.a <= 0.5f)
    {
        discard;
    }

    // ライティングが無効な場合の早期リターン
    if (gMaterial.enableLighting == 0)
    {
        output.color = gMaterial.color * textureColor;
        return output;
    }

    // 共通計算（一度だけ実行）
    float3 normal = normalize(input.normal);
    float3 toEye = normalize(gCamera.worldPos - input.worldPos);
    float3 baseColor = gMaterial.color.rgb * textureColor.rgb;

    // シャドウ計算（カスケードシャドウを使用）
    float shadow = 1.0f;
    if (gCascadeShadowData.enableShadow)
    {
        // ビュー空間での深度を計算（カスケード選択用）
        float3 viewPos = input.worldPos - gCamera.worldPos;
        float viewDepth = length(viewPos);
        
        shadow = CalculateCascadeShadow(input.worldPos, viewDepth);
    }
    else if (gShadowData.enableShadow)
    {
        // フォールバック: 旧単一シャドウマップ
        shadow = CalculateShadow(input.worldPos);
    }

    /*-----[ ディレクショナルライト ]-----*/
    float3 lightDir = normalize(-gDirectionalLight.direction);
    float NdotL = CalculateHalfLambert(normal, lightDir);
    // シャドウを適用
    float3 diffuse = baseColor * gDirectionalLight.color.rgb * NdotL * gDirectionalLight.intensity * shadow;
    float3 specular = CalculateSpecular(normal, lightDir, toEye, gDirectionalLight.color.rgb, gDirectionalLight.intensity, gMaterial.shininess) * shadow;

    /*-----[ ポイントライトの合計（最適化されたループ）]-----*/
    float3 totalPointDiffuse = 0.0f;
    float3 totalPointSpecular = 0.0f;
    
    [loop]
    for (uint j = 0; j < gLightCounts.gPointLightCount; j++)
    {
        float3 lightToPixel = input.worldPos - gPointLights[j].position;
        float distance = length(lightToPixel);
        float3 pointLightDir = lightToPixel / distance; // 正規化を効率化
        
        // 減衰計算
        float factor = pow(saturate(1.0f - distance / gPointLights[j].radius), gPointLights[j].decay);
        
        // 減衰が十分小さい場合はスキップ
        if (factor < 0.01f)
            continue;
        
        float pointNdotL = CalculateHalfLambert(normal, -pointLightDir);
        float3 pointDiffuse = baseColor * gPointLights[j].color.rgb * pointNdotL * gPointLights[j].intensity * factor;
        float3 pointSpecular = CalculateSpecular(normal, -pointLightDir, toEye, gPointLights[j].color.rgb, gPointLights[j].intensity, gMaterial.shininess) * factor;
        
        totalPointDiffuse += pointDiffuse;
        totalPointSpecular += pointSpecular;
    }

    /*-----[ スポットライトの合計（最適化されたループ）]-----*/
    float3 totalSpotDiffuse = 0.0f;
    float3 totalSpotSpecular = 0.0f;
    
    [loop]
    for (uint k = 0; k < gLightCounts.gSpotLightCount; k++)
    {
        float3 lightToPixel = input.worldPos - gSpotLights[k].position;
        float distance = length(lightToPixel);
        float3 spotLightDir = lightToPixel / distance; // 正規化を効率化
        
        // 距離減衰
        float spotFactor = pow(saturate(1.0f - distance / gSpotLights[k].distance), gSpotLights[k].decay);
        
        // フォールオフ計算
        float cosAngle = dot(spotLightDir, gSpotLights[k].direction);
        float falloffFactor = saturate((cosAngle - gSpotLights[k].cosAngle) / (gSpotLights[k].cosFalloffStart - gSpotLights[k].cosAngle));
        
        float combinedFactor = spotFactor * falloffFactor;
        
        // 結合された減衰が十分小さい場合はスキップ
        if (combinedFactor < 0.01f)
            continue;
        
        float spotNdotL = CalculateHalfLambert(normal, -spotLightDir);
        float3 spotDiffuse = baseColor * gSpotLights[k].color.rgb * spotNdotL * gSpotLights[k].intensity * combinedFactor;
        float3 spotSpecular = CalculateSpecular(normal, -spotLightDir, toEye, gSpotLights[k].color.rgb, gSpotLights[k].intensity, gMaterial.shininess) * combinedFactor;
        
        totalSpotDiffuse += spotDiffuse;
        totalSpotSpecular += spotSpecular;
    }

    // ライティング結果の合成
    float3 litColor = diffuse + specular + totalPointDiffuse + totalPointSpecular + totalSpotDiffuse + totalSpotSpecular;

    // 環境マッピング（reflectivityが0でない場合のみ）
    float3 finalColor = litColor;
    if (gMaterial.reflectivity > 0.0f)
    {
        float3 reflectDir = reflect(-toEye, normal);
        float3 envColor = gEnvironmentTexture.Sample(gSampler, reflectDir).rgb;
        finalColor = lerp(litColor, envColor, gMaterial.reflectivity);
    }

    output.color.rgb = finalColor;
    output.color.a = gMaterial.color.a * textureColor.a;

    return output;
}
