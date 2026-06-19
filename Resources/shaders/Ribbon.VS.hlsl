// Ribbon.VS.hlsl
// リボンレンダラー用の頂点シェーダー

#include "Ribbon.hlsli"

// リボン頂点入力構造体 (RibbonVertexと一致)
struct VertexShaderInput
{
    float3 position : POSITION0;  // Vector3
    float2 texcoord : TEXCOORD0;  // Vector2
    float4 color : COLOR0;        // Vector4
};

// ビュー/プロジェクション行列
cbuffer ViewProjection : register(b0)
{
    float4x4 viewProjection;
}

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // リボンは既にワールド座標で頂点が用意されているため、
    // ビュープロジェクション変換のみ行う
    output.position = mul(float4(input.position, 1.0f), viewProjection);
    output.texcoord = input.texcoord;
    output.color = input.color;
    
    return output;
}
