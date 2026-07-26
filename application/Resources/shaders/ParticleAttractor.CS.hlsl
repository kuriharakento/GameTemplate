// GPUパーティクル 引力(Attractor)用コンピュートシェーダー

struct Particle
{
    float3 position;    float pad0;
    float3 velocity;    float pad1;
    float3 scale;       float pad2;
    float4 rotation;
    float4 color;
    float4 initialColor;
    float age;          float lifetime;     float ribbonWidth;  uint flags;
    uint id;            uint ribbonId;      uint spriteIndex;   uint pad3;
};

cbuffer AttractorParams : register(b0)
{
    float3 target;
    float strength;
    float radius;
    uint falloffType;
    uint particleCount;
    float deltaTime;
};

RWStructuredBuffer<Particle> particles : register(u0);

float CalculateFalloff(float distance)
{
    if (falloffType == 0) // None
    {
        return 1.0f;
    }
    else if (falloffType == 1) // Linear
    {
        return (radius > 0.0f) ? max(0.0f, 1.0f - distance / radius) : 1.0f;
    }
    else if (falloffType == 2) // InverseSquare
    {
        return 1.0f / (1.0f + distance * distance);
    }
    return 1.0f;
}

[numthreads(256, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint id = dtid.x;
    if (id >= particleCount) return;

    Particle p = particles[id];
    
    // 生存チェック
    if ((p.flags & 1) == 0) return;

    float3 direction = target - p.position;
    float distance = length(direction);

    if (distance < 0.001f) return;

    // 正規化
    direction /= distance;

    // 減衰計算
    float force = CalculateFalloff(distance);

    // 速度に加算
    p.velocity += direction * force * strength * deltaTime;

    // 書き戻し
    particles[id] = p;
}
