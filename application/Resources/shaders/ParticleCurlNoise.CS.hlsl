// GPUパーティクル カールノイズ(CurlNoise)用コンピュートシェーダー

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

cbuffer CurlNoiseParams : register(b0)
{
    float strength;
    float frequency;
    int octaves;
    float scrollSpeed;
    uint particleCount;
    float deltaTime;
    uint2 padding;
};

RWStructuredBuffer<Particle> particles : register(u0);

float Noise3D(float3 p, int octs)
{
    float value = 0.0f;
    float amplitude = 1.0f;
    for (int i = 0; i < octs; ++i)
    {
        // CPU側の std::sin(x * 1.7 + y * 2.3 + z * 3.1 + i * 1.3) のアルゴリズムを再現
        value += sin(p.x * 1.7f + p.y * 2.3f + p.z * 3.1f + float(i) * 1.3f) * amplitude;
        p *= 2.0f;
        amplitude *= 0.5f;
    }
    return value;
}

float3 ComputeCurlNoise(float3 p, float time)
{
    float eps = 0.001f;
    float3 pos = p * frequency;
    pos.y += time;

    float n1 = Noise3D(pos, octaves);
    float dnx = Noise3D(pos + float3(eps, 0.0f, 0.0f), octaves) - n1;
    float dny = Noise3D(pos + float3(0.0f, eps, 0.0f), octaves) - n1;
    float dnz = Noise3D(pos + float3(0.0f, 0.0f, eps), octaves) - n1;

    // カール計算: curl(F) = (dFz/dy - dFy/dz, dFx/dz - dFz/dx, dFy/dx - dFx/dy)
    return float3(dny - dnz, dnz - dnx, dnx - dny);
}

[numthreads(256, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint id = dtid.x;
    if (id >= particleCount) return;

    Particle p = particles[id];
    
    // 生存チェック
    if ((p.flags & 1) == 0) return;

    float3 curl = ComputeCurlNoise(p.position, p.age * scrollSpeed);

    p.velocity += curl * strength * deltaTime;

    // 書き戻し
    particles[id] = p;
}
