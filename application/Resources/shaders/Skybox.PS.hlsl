#include "Skybox.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
    float shininess;
};

ConstantBuffer<Material> gMaterial : register(b0);
TextureCube<float4> gSkyboxTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float32_t4 textureColor = gSkyboxTexture.Sample(gSampler, input.texcoord);
    output.color = gMaterial.color * textureColor;
    return output;
}
