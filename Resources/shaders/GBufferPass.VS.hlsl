// G-Buffer Pass Vertex Shader
// ジオメトリパス用頂点シェーダー（Object3d.VS.hlslベース）

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPos : TEXCOORD1;
};

// row_major を追加（C++構造体と同じ格納順序）
cbuffer TransformationMatrix : register(b0)
{
    row_major float4x4 WVP;
    row_major float4x4 World;
    row_major float4x4 WorldInverseTranspose;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // クリップ座標（Object3d.VS.hlslと同じ）
    output.position = mul(input.position, WVP);
    
    // テクスチャ座標
    output.texcoord = input.texcoord;
    
    // 法線をワールド空間に変換
    output.normal = normalize(mul(input.normal, (float3x3) WorldInverseTranspose));
    
    // ワールド座標
    output.worldPos = mul(input.position, World).xyz;
    
    return output;
}
