#include "Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

// インスタンスごとの行列バッファ
StructuredBuffer<TransformationMatrix> gInstanceMatrices : register(t0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    
    // インスタンスIDを使用して行列を取得
    float4x4 worldMatrix = gInstanceMatrices[instanceId].World;
    float4x4 wvpMatrix = gInstanceMatrices[instanceId].WVP;
    float4x4 worldInverseTranspose = gInstanceMatrices[instanceId].WorldInverseTranspose;

    output.position = mul(input.position, wvpMatrix);
    output.worldPos = mul(input.position, worldMatrix).xyz;
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3)worldInverseTranspose));
    
    return output;
}
