// Instanced Shadow Map Vertex Shader

struct VertexShaderInput
{
    float4 position : POSITION0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
};

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

// ライトのビュー・プロジェクション行列
cbuffer LightViewProjection : register(b0)
{
    float4x4 gLightViewProjection;
};

StructuredBuffer<TransformationMatrix> gInstanceMatrices : register(t0);

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    
    // インスタンスのワールド行列を取得
    float4x4 worldMatrix = gInstanceMatrices[instanceId].World;
    
    // ワールド座標に変換
    float4 worldPos = mul(input.position, worldMatrix);
    
    // ライト空間座標に変換
    output.position = mul(worldPos, gLightViewProjection);
    
    return output;
}
