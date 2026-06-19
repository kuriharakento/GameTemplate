// Light Pass Pixel Shader
// ディファードライティング（ディレクショナル、スポット、ポイント対応）

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// 定数
#define MAX_SPOT_LIGHTS 8
#define MAX_POINT_LIGHTS 2

// G-Bufferテクスチャ
Texture2D<float4> gAlbedo : register(t0);
Texture2D<float4> gNormal : register(t1);
Texture2D<float4> gMaterial : register(t2);
Texture2D<float4> gEmissive : register(t3);
Texture2D<float> gDepth : register(t4);

// カスケードシャドウマップ
Texture2D<float> gCascadeShadow0 : register(t5);
Texture2D<float> gCascadeShadow1 : register(t6);
Texture2D<float> gCascadeShadow2 : register(t7);
Texture2D<float> gCascadeShadow3 : register(t8);

// スポットライトシャドウマップ（8個）
Texture2D<float> gSpotShadow0 : register(t9);
Texture2D<float> gSpotShadow1 : register(t10);
Texture2D<float> gSpotShadow2 : register(t11);
Texture2D<float> gSpotShadow3 : register(t12);
Texture2D<float> gSpotShadow4 : register(t13);
Texture2D<float> gSpotShadow5 : register(t14);
Texture2D<float> gSpotShadow6 : register(t15);
Texture2D<float> gSpotShadow7 : register(t16);

// ポイントライトシャドウマップ（キューブマップ）
TextureCube<float> gPointShadow0 : register(t17);
TextureCube<float> gPointShadow1 : register(t18);

SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

// カメラ定数
cbuffer CameraData : register(b0)
{
    float3 cameraWorldPos;
    float padding0;
    row_major float4x4 viewMatrix;
    row_major float4x4 projMatrix;
    row_major float4x4 invViewMatrix;
    row_major float4x4 invProjMatrix;
    float nearPlane;
    float farPlane;
    float2 padding1;
};

// ディレクショナルライト
cbuffer DirectionalLightData : register(b1)
{
    float4 dirLightColor;
    float3 dirLightDirection;
    float dirLightIntensity;
};

// カスケードシャドウデータ
cbuffer CascadeShadowData : register(b2)
{
    row_major float4x4 cascadeViewProj[4];
    float4 cascadeSplits;
    int enableCascadeShadow;
    float3 cascadePadding;
};

// スポットライト構造体
struct SpotLight
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
    float4 padding; // 16バイトアライメント用パディング
    row_major float4x4 shadowViewProj;
};

// ポイントライト構造体
struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
    float radius;
    float decay;
    int shadowEnabled;
    float padding;
    row_major float4x4 shadowViewProj[6];
};

// ライトバッファ
cbuffer LightBuffer : register(b3)
{
    int numSpotLights;
    int numPointLights;
    float2 lightPadding;
    SpotLight spotLights[MAX_SPOT_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
};

// 法線デコード
float3 DecodeNormal(float3 encoded)
{
    return encoded * 2.0f - 1.0f;
}

// ワールド座標を再構築
float3 ReconstructWorldPosition(float2 texcoord, float depth)
{
    float4 clipPos;
    clipPos.x = texcoord.x * 2.0f - 1.0f;
    clipPos.y = (1.0f - texcoord.y) * 2.0f - 1.0f;
    clipPos.z = depth;
    clipPos.w = 1.0f;
    
    float4 viewPos = mul(clipPos, invProjMatrix);
    viewPos /= viewPos.w;
    
    float4 worldPos = mul(viewPos, invViewMatrix);
    return worldPos.xyz;
}

// カスケードシャドウ計算
float CalculateCascadeShadow(float3 worldPos, float viewDepth)
{
    if (enableCascadeShadow == 0)
        return 1.0f;
    
    int cascadeIndex = 3;
    for (int i = 0; i < 4; ++i)
    {
        if (viewDepth < cascadeSplits[i])
        {
            cascadeIndex = i;
            break;
        }
    }
    
    float4 lightSpacePos = mul(float4(worldPos, 1.0f), cascadeViewProj[cascadeIndex]);
    lightSpacePos /= lightSpacePos.w;
    
    float2 shadowUV = lightSpacePos.xy * 0.5f + 0.5f;
    shadowUV.y = 1.0f - shadowUV.y;
    
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f || shadowUV.y < 0.0f || shadowUV.y > 1.0f)
        return 1.0f;
    
    float currentDepth = lightSpacePos.z;
    float bias = 0.0005f;
    
    float shadow = 0.0f;
    float2 texelSize = 1.0f / 2048.0f;
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            
            if (cascadeIndex == 0)
                shadow += gCascadeShadow0.SampleCmpLevelZero(gShadowSampler, shadowUV + offset, currentDepth - bias);
            else if (cascadeIndex == 1)
                shadow += gCascadeShadow1.SampleCmpLevelZero(gShadowSampler, shadowUV + offset, currentDepth - bias);
            else if (cascadeIndex == 2)
                shadow += gCascadeShadow2.SampleCmpLevelZero(gShadowSampler, shadowUV + offset, currentDepth - bias);
            else
                shadow += gCascadeShadow3.SampleCmpLevelZero(gShadowSampler, shadowUV + offset, currentDepth - bias);
        }
    }
    
    return shadow / 9.0f;
}

// スポットライトシャドウ計算
float CalculateSpotShadow(float3 worldPos, int lightIndex, float4x4 shadowVP, float3 normal, float3 lightDir)
{
    float4 lightSpacePos = mul(float4(worldPos, 1.0f), shadowVP);
    lightSpacePos /= lightSpacePos.w;
    
    float2 shadowUV = lightSpacePos.xy * 0.5f + 0.5f;
    shadowUV.y = 1.0f - shadowUV.y;
    
    // UV座標の範囲外チェック
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f || shadowUV.y < 0.0f || shadowUV.y > 1.0f)
        return 1.0f;
    
    float currentDepth = lightSpacePos.z;
    
    // 深度(Z)の範囲外チェック（near plane前、またはfar plane後ろ）
    if (currentDepth < 0.0f || currentDepth > 1.0f)
        return 1.0f;
    
    // Adaptive Bias: 法線の角度と深度に応じてバイアスを調整
    // 深度が1.0に近いほど（遠いほど）バイアスを大きくして精度問題を緩和
    float depthBiasFactor = lerp(1.0f, 3.0f, currentDepth);
    float bias = max(0.002f * (1.0f - dot(normal, lightDir)) * depthBiasFactor, 0.001f);
    
    float shadow = 0.0f;
    if (lightIndex == 0)
        shadow = gSpotShadow0.SampleCmpLevelZero(gShadowSampler, shadowUV, currentDepth - bias);
    else if (lightIndex == 1)
        shadow = gSpotShadow1.SampleCmpLevelZero(gShadowSampler, shadowUV, currentDepth - bias);
    else if (lightIndex == 2)
        shadow = gSpotShadow2.SampleCmpLevelZero(gShadowSampler, shadowUV, currentDepth - bias);
    else if (lightIndex == 3)
        shadow = gSpotShadow3.SampleCmpLevelZero(gShadowSampler, shadowUV, currentDepth - bias);
    else if (lightIndex == 4)
        shadow = gSpotShadow4.SampleCmpLevelZero(gShadowSampler, shadowUV, currentDepth - bias);
    else if (lightIndex == 5)
        shadow = gSpotShadow5.SampleCmpLevelZero(gShadowSampler, shadowUV, currentDepth - bias);
    else if (lightIndex == 6)
        shadow = gSpotShadow6.SampleCmpLevelZero(gShadowSampler, shadowUV, currentDepth - bias);
    else
        shadow = gSpotShadow7.SampleCmpLevelZero(gShadowSampler, shadowUV, currentDepth - bias);
    
    return shadow;
}

// ポイントライトシャドウ計算
float CalculatePointShadow(float3 worldPos, int lightIndex, float3 lightPos, float lightRadius)
{
    float3 lightToFrag = worldPos - lightPos;
    float3 dir = normalize(lightToFrag);
    
    // ハードウェアZと比較するための非線形深度を計算
    // Cubemapの各面のローカルZ（視線方向の距離）を求める
    float3 absVec = abs(lightToFrag);
    float localZ = max(absVec.x, max(absVec.y, absVec.z));
    
    const float nearPlane = 0.1f;
    float farPlane = lightRadius;
    
    // プロジェクション変換後のZ値 (0-1)
    // depth = f/(f-n) - (f*n)/(f-n)/z
    float term1 = farPlane / (farPlane - nearPlane);
    float term2 = (farPlane * nearPlane) / (farPlane - nearPlane);
    float nonLinearDepth = term1 - (term2 / localZ);
    
    float bias = 0.005f;
    float shadow = 0.0f;
    
    if (lightIndex == 0)
        shadow = gPointShadow0.SampleCmpLevelZero(gShadowSampler, dir, nonLinearDepth - bias);
    else
        shadow = gPointShadow1.SampleCmpLevelZero(gShadowSampler, dir, nonLinearDepth - bias);
    
    return shadow;
}

// スポットライトの減衰計算
float3 CalculateSpotLight(SpotLight light, float3 worldPos, float3 normal, float3 albedo, int lightIndex)
{
    float3 lightVec = light.position - worldPos;
    float dist = length(lightVec);
    
    if (dist > light.distance)
        return float3(0, 0, 0);
    
    float3 lightDir = lightVec / dist;
    
    // 円錐減衰
    float cosTheta = dot(-lightDir, normalize(light.direction));
    if (cosTheta < light.cosAngle)
        return float3(0, 0, 0);
    
    float spotFactor = saturate((cosTheta - light.cosAngle) / (light.cosFalloffStart - light.cosAngle));
    
    // 距離減衰
    float attenuation = pow(saturate(1.0f - dist / light.distance), light.decay);
    
    // ディフューズ
    float NdotL = max(dot(normal, lightDir), 0.0f);
    
    // シャドウ
    float shadow = 1.0f;
    if (light.shadowEnabled != 0)
    {
        shadow = CalculateSpotShadow(worldPos, lightIndex, light.shadowViewProj, normal, lightDir);
    }
    
    return albedo * NdotL * light.color.rgb * light.intensity * attenuation * spotFactor * shadow;
}

// ポイントライトの計算
float3 CalculatePointLight(PointLight light, float3 worldPos, float3 normal, float3 albedo, int lightIndex)
{
    float3 lightVec = light.position - worldPos;
    float dist = length(lightVec);
    
    if (dist > light.radius)
        return float3(0, 0, 0);
    
    float3 lightDir = lightVec / dist;
    
    // 距離減衰
    float attenuation = pow(saturate(1.0f - dist / light.radius), light.decay);
    
    // ディフューズ
    float NdotL = max(dot(normal, lightDir), 0.0f);
    
    // シャドウ
    float shadow = 1.0f;
    if (light.shadowEnabled != 0)
    {
        shadow = CalculatePointShadow(worldPos, lightIndex, light.position, light.radius);
    }
    
    return albedo * NdotL * light.color.rgb * light.intensity * attenuation * shadow;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    // G-Bufferからデータ取得
    float4 albedoMetal = gAlbedo.Sample(gSampler, input.texcoord);
    float4 normalData = gNormal.Sample(gSampler, input.texcoord);
    float4 materialData = gMaterial.Sample(gSampler, input.texcoord);
    float depth = gDepth.Sample(gSampler, input.texcoord);
    
    // 背景をスキップ
    if (depth >= 1.0f)
    {
        return float4(0.1f, 0.1f, 0.15f, 1.0f);
    }
    
    float3 albedo = albedoMetal.rgb;
    float3 normal = normalize(DecodeNormal(normalData.rgb));
    float ao = materialData.g;
    
    float3 worldPos = ReconstructWorldPosition(input.texcoord, depth);
    
    float4 viewPos = mul(float4(worldPos, 1.0f), viewMatrix);
    float viewDepth = viewPos.z;
    
    // アンビエント
    float3 ambient = albedo * 0.1f * ao;
    
    // ディレクショナルライト
    float3 lightDir = normalize(-dirLightDirection);
    float NdotL = max(dot(normal, lightDir), 0.0f);
    float dirShadow = CalculateCascadeShadow(worldPos, viewDepth);
    float3 directional = albedo * NdotL * dirLightColor.rgb * dirLightIntensity * dirShadow;
    
    // スポットライト
    float3 spotContribution = float3(0, 0, 0);
    for (int s = 0; s < numSpotLights && s < MAX_SPOT_LIGHTS; ++s)
    {
        spotContribution += CalculateSpotLight(spotLights[s], worldPos, normal, albedo, s);
    }
    
    // ポイントライト
    float3 pointContribution = float3(0, 0, 0);
    for (int p = 0; p < numPointLights && p < MAX_POINT_LIGHTS; ++p)
    {
        pointContribution += CalculatePointLight(pointLights[p], worldPos, normal, albedo, p);
    }
    
    float3 finalColor = ambient + directional + spotContribution + pointContribution;
    
    return float4(finalColor, 1.0f);
}
