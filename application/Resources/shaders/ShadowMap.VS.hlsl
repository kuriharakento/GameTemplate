/**
 * @file ShadowMap.VS.hlsl
 * @brief シャドウマップ描画用頂点シェーダー
 * @details ライトの視点からオブジェクトを描画するためのシンプルな頂点変換
 */

// 頂点入力構造体
struct VertexShaderInput
{
    float4 position : POSITION0;
};

// 頂点出力構造体
struct VertexShaderOutput
{
    float4 position : SV_POSITION;
};

// ライトのビュー・プロジェクション行列
cbuffer LightViewProjection : register(b0)
{
    float4x4 gLightViewProjection;
};

// オブジェクトのワールド行列
cbuffer WorldMatrix : register(b1)
{
    float4x4 gWVP; // TransformationMatrix.WVP (使用しない)
    float4x4 gWorld; // TransformationMatrix.World (使用する)
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // ワールド座標に変換
    float4 worldPos = mul(input.position, gWorld);
    
    // ライト空間座標に変換
    output.position = mul(worldPos, gLightViewProjection);
    
    return output;
}
