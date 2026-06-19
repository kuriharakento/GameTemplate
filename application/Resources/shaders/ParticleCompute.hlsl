/**
 * ParticleCompute.hlsl
 * GPUパーティクルシミュレーション用 Compute Shader
 * 
 * C++側のParticle構造体と完全に同じレイアウト (112 bytes, 16-byte aligned)
 */

// パーティクル構造体（C++Particle構造体と完全一致 - 128 bytes）
struct Particle
{
    // Transform (48 bytes)
    float3 position;    // 12 bytes
    float pad0;         // 4 bytes
    
    float3 velocity;    // 12 bytes
    float pad1;         // 4 bytes
    
    float3 scale;       // 12 bytes
    float pad2;         // 4 bytes
    
    // Rotation (16 bytes)
    float4 rotation;    // Quaternion XYZW
    
    // Appearance (16 bytes)
    float4 color;       // RGBA
    
    // Initial Color (16 bytes)
    float4 initialColor; // InitialColorModuleで設定された初期カラー
    
    // Lifetime (16 bytes)
    float age;          // 経過時間
    float lifetime;     // 寿命
    float ribbonWidth;  // リボン幅
    uint flags;         // ビットフラグ
    
    // IDs (16 bytes)
    uint id;            // パーティクルID
    uint ribbonId;      // リボングループID
    uint spriteIndex;   // テクスチャシートフレーム
    uint pad3;          // アライメント
};

// 定数バッファ（C++側のGPUParticleConstantsと一致）
cbuffer Constants : register(b0)
{
    float deltaTime;
    float totalTime;
    uint particleCount;
    uint maxParticles;
    
    float3 emitterPosition;
    float padding1;
    
    float3 gravity;
    float padding2;
};

// パーティクルバッファ (UAV)
RWStructuredBuffer<Particle> particles : register(u0);

// フラグ定数
static const uint FLAG_ALIVE = 1 << 0;
static const uint FLAG_RIBBON_HEAD = 1 << 1;

/**
 * メインシミュレーションカーネル
 */
[numthreads(256, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint index = dispatchThreadId.x;
    
    // 範囲外チェック
    if (index >= particleCount)
        return;
    
    Particle p = particles[index];
    
    // 死亡パーティクルはスキップ
    if ((p.flags & FLAG_ALIVE) == 0)
        return;
    
    // 年齢を更新
    p.age += deltaTime;
    
    // 寿命チェック
    if (p.age >= p.lifetime)
    {
        p.flags &= ~FLAG_ALIVE;
        particles[index] = p;
        return;
    }
    
    // 重力を適用
    p.velocity += gravity * deltaTime;
    
    // 位置を更新
    p.position += p.velocity * deltaTime;
    
    // カラーフェード（寿命に応じてアルファを減少）
    float lifeRatio = p.age / p.lifetime;
    p.color.a = saturate(1.0f - lifeRatio);
    
    // 結果を書き戻し
    particles[index] = p;
}
