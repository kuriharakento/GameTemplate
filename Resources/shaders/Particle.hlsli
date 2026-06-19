struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

// GPUシミュレーション用パーティクルデータ構造体
struct ParticleForGPU
{
    float4x4 WVP;
    float4x4 World;
    float4 color;
    float4 uvOffsetScale; // テクスチャシート用 (offsetX, offsetY, scaleX, scaleY)
};
