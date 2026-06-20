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
    uint isBillboard;
    
    float3 gravity;
    uint simulationSpace;

    float4x4 emitterWorld;

    // 追加モジュールパラメータ (アプローチB)
    uint hasDrag;
    float dragMin;
    float dragMax;
    float paddingDrag;
    
    uint hasColorFade;
    uint colorFadeUseInitial;
    uint colorFadeEasing;
    float paddingCF;
    float4 colorFadeStart;
    float4 colorFadeEnd;
    
    uint hasScaleOL;
    uint scaleOLEasing;
    float2 paddingScaleOL;
    float3 scaleOLStart;
    float paddingS1;
    float3 scaleOLEnd;
    float paddingS2;

    // Noise
    uint hasNoise;
    float noiseStrength;
    float noiseFrequency;
    float paddingNoise;

    // RotationOverLifetime
    uint hasRotationOL;
    float rotOLStartSpeed;
    float rotOLEndSpeed;
    uint rotOLEasing;

    // AlphaFade
    uint hasAlphaFade;
    float alphaFadeStart;
    float alphaFadeEnd;
    uint alphaFadeEaseIn;
    uint alphaFadeEaseOut;
    float3 paddingAlpha;

    // VelocityOverLifetime
    uint hasVelocityOL;
    float velocityOLStart;
    float velocityOLEnd;
    float paddingVelocityOL;

    // StretchByVelocity
    uint hasStretchByVelocity;
    float stretchFactor;
    float minStretch;
    float maxStretch;
    uint stretchPreserveVolume;
    float3 paddingStretch;

    // Flicker
    uint hasFlicker;
    float flickerFrequency;
    float flickerMinAlpha;
    float flickerMaxAlpha;
    uint flickerRandomPhase;
    uint flickerUseNoise;
    float2 paddingFlicker;

    // FaceVelocity
    uint hasFaceVelocity;
    uint faceVelocityUse2D;
    float2 paddingFaceVelocity;
};

// パーティクルバッファ (UAV)
RWStructuredBuffer<Particle> particles : register(u0);

// フラグ定数
static const uint FLAG_ALIVE = 1 << 0;
static const uint FLAG_RIBBON_HEAD = 1 << 1;

// イージング関数
float ApplyEasing(uint type, float t)
{
    if (type == 0) return t; // Linear
    if (type == 1) return 1.0f - cos(t * 1.5707963f); // EaseInSine
    if (type == 2) return sin(t * 1.5707963f); // EaseOutSine
    if (type == 3) return -(cos(3.1415926f * t) - 1.0f) * 0.5f; // EaseInOutSine
    if (type == 4) return t * t; // EaseInQuad
    if (type == 5) return t * (2.0f - t); // EaseOutQuad
    if (type == 6) return t < 0.5f ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) * 0.5f; // EaseInOutQuad
    return t;
}

// 決定論的乱数 (C++と同じハッシュ関数)
float DeterministicRandom(uint id, uint subSeed)
{
    uint x = id + subSeed * 0x9e3779b9u;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = (x >> 16) ^ x;
    return float(x) / 4294967295.0f;
}

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
    
    float lifeRatio = saturate(p.age / p.lifetime);

    // 1. DragModule (空気抵抗)
    if (hasDrag != 0)
    {
        float r = DeterministicRandom(p.id, 0); // subSeed = 0
        float d = lerp(dragMin, dragMax, r);
        float factor = 1.0f - d * deltaTime;
        p.velocity *= saturate(factor);
    }

    // 1.2 VelocityOverLifetimeModule (寿命に応じた速度乗算)
    if (hasVelocityOL != 0)
    {
        float multiplier = lerp(velocityOLStart, velocityOLEnd, lifeRatio);
        float dampFactor = 1.0f - (1.0f - multiplier) * deltaTime;
        p.velocity *= dampFactor;
    }
    
    // 重力を適用
    p.velocity += gravity * deltaTime;
    
    // 位置を更新
    p.position += p.velocity * deltaTime;
    
    // 2. ColorFadeModule
    if (hasColorFade != 0)
    {
        float t = ApplyEasing(colorFadeEasing, lifeRatio);
        float4 effectiveStart = (colorFadeUseInitial != 0) ? p.initialColor : colorFadeStart;
        p.color = lerp(effectiveStart, colorFadeEnd, t);
    }
    else
    {
        // デフォルトのカラーフェード（寿命に応じてアルファを減少）
        p.color.a = saturate(1.0f - lifeRatio);
    }

    // 3. ScaleOverLifetimeModule
    if (hasScaleOL != 0)
    {
        float t = ApplyEasing(scaleOLEasing, lifeRatio);
        p.scale = lerp(scaleOLStart, scaleOLEnd, t);
    }

    // 3.5. StretchByVelocityModule (速度によるスケール伸長)
    if (hasStretchByVelocity != 0)
    {
        float speed = length(p.velocity);
        float stretch = 1.0f + speed * stretchFactor;
        stretch = clamp(stretch, minStretch, maxStretch);
        p.scale.y = stretch;
        if (stretchPreserveVolume != 0)
        {
            float shrink = 1.0f / sqrt(stretch);
            p.scale.x = shrink;
            p.scale.z = shrink;
        }
    }

    // 4. NoiseModule (シンプルなサイン波ノイズ風の動き)
    if (hasNoise != 0)
    {
        float t = p.age * noiseFrequency;
        float idOffset = float(p.id);
        float3 noiseVal = float3(
            sin(t * 2.0f + idOffset * 0.1f) * noiseStrength,
            sin(t * 2.3f + idOffset * 0.2f) * noiseStrength,
            sin(t * 2.7f + idOffset * 0.3f) * noiseStrength
        );
        p.velocity += noiseVal * deltaTime;
    }

    // 4.5. FaceVelocityModule (進行方向アライメント)
    if (hasFaceVelocity != 0)
    {
        float speedSq = dot(p.velocity, p.velocity);
        if (speedSq > 0.0001f)
        {
            if (faceVelocityUse2D != 0)
            {
                p.rotation.z = atan2(p.velocity.y, p.velocity.x) - (3.14159265f * 0.5f);
            }
            else
            {
                float3 normDirection = normalize(p.velocity);
                float yaw = atan2(normDirection.x, normDirection.z);
                float pitch = atan2(normDirection.y, sqrt(normDirection.x * normDirection.x + normDirection.z * normDirection.z));
                p.rotation.x = -pitch;
                p.rotation.y = yaw;
                p.rotation.z = 0.0f;
            }
        }
    }

    // 5. RotationOverLifetimeModule (回転速度のイージング変化と加算)
    if (hasRotationOL != 0)
    {
        float t = ApplyEasing(rotOLEasing, lifeRatio);
        float speed = lerp(rotOLStartSpeed, rotOLEndSpeed, t);
        // Z軸まわりの回転（ラジアンへ変換して加算）
        float angleRad = speed * deltaTime * (3.14159265f / 180.0f);
        p.rotation.z += angleRad;
    }

    // 6. AlphaFadeModule (アルファ値のみをシンプルにフェード)
    if (hasAlphaFade != 0)
    {
        float t = lifeRatio;
        if (alphaFadeEaseIn != 0 && alphaFadeEaseOut != 0)
        {
            t = t * t * (3.0f - 2.0f * t); // smoothstep
        }
        else if (alphaFadeEaseIn != 0)
        {
            t = t * t;
        }
        else if (alphaFadeEaseOut != 0)
        {
            t = 1.0f - (1.0f - t) * (1.0f - t);
        }
        p.color.a = lerp(alphaFadeStart, alphaFadeEnd, t);
    }

    // 6.5. FlickerModule (アルファ値の点滅)
    if (hasFlicker != 0)
    {
        float t = p.age * flickerFrequency;
        if (flickerRandomPhase != 0)
        {
            t += float(p.id) * 0.1f;
        }
        
        float alphaVal = 0.0f;
        if (flickerUseNoise != 0)
        {
            // ノイズベース
            alphaVal = (sin(t * 2.0f) + sin(t * 3.7f) + 2.0f) * 0.25f;
        }
        else
        {
            // シンプルなサイン波
            alphaVal = (sin(t * 3.14159265f * 2.0f) + 1.0f) * 0.5f;
        }
        p.color.a = flickerMinAlpha + (flickerMaxAlpha - flickerMinAlpha) * alphaVal;
    }
    
    // 結果を書き戻し
    particles[index] = p;
}
