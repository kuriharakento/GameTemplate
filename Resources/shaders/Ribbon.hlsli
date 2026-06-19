// Ribbon.hlsli
// リボンレンダラー用の共通構造体定義

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};
