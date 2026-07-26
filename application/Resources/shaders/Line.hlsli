// 定数バッファ（ワールドビュー・プロジェクション行列）
cbuffer WVPBuffer : register(b0)
{
    matrix WVP; // ワールド・ビュー・プロジェクション行列
};

// 頂点データ構造体
struct VertexInput
{
    float3 position;
    float4 color;
};

// ピクセルシェーダーへ渡す構造体
struct PixelInput
{
    float4 position;
    float4 color;
};