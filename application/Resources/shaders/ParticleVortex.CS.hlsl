// GPUパーティクル 渦(Vortex)用コンピュートシェーダー

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

cbuffer VortexParams : register(b0)
{
    float3 axis;
    float strength;
    float3 center;
    float radius;
    uint falloffType;
    float deltaTime;
    uint particleCount;
    uint padding;
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

    // 中心から粒子へのベクトル
    float3 toParticle = p.position - center;

    // 外積で接線方向を計算
    float3 tangent;
    tangent.x = axis.y * toParticle.z - axis.z * toParticle.y;
    tangent.y = axis.z * toParticle.x - axis.x * toParticle.z;
    tangent.z = axis.x * toParticle.y - axis.y * toParticle.x;

    // 減衰計算
    float distance = length(toParticle);
    float force = CalculateFalloff(distance);

    // 速度に加算
    p.velocity += tangent * force * strength * deltaTime;

    // 書き戻し
    particles[id] = p;
}
