#include "Particle.hlsli"

// シミュレーション用パーティクル構造体 (128 bytes - CPU Particle構造体と一致)
struct Particle
{
    float3 position; float pad0;
    float3 velocity; float pad1;
    float3 scale;    float pad2;
    float4 rotation;
    float4 color;
    float4 initialColor; // InitialColorModuleで設定された初期カラー
    float age; float lifetime; float ribbonWidth; uint flags;
    uint id; uint ribbonId; uint spriteIndex; uint pad3;
};

// カメラ情報
struct Camera
{
    float4x4 view;
    float4x4 projection;
    float3 eye;
    float pad;
};

// 入力：シミュレーション結果
StructuredBuffer<Particle> gParticles : register(t0);
// 出力：レンダリング用データ
RWStructuredBuffer<ParticleForGPU> gRenderParticles : register(u0);

// 定数
cbuffer Constants : register(b0)
{
    float deltaTime;
    float totalTime;
    uint particleCount;
    uint maxParticles;
    
    float3 emitterPosition;
    float padding1;
    
    float3 gravity;
    uint isBillboard;
}

// カメラ定数
cbuffer CameraBuffer : register(b1)
{
    float4x4 cameraView;
    float4x4 cameraProjection;
    float3 cameraEye;
    float cameraPad;
}

// クォータニオン -> 回転行列
float4x4 QuaternionToMatrix(float4 q)
{
    float x2 = q.x + q.x; float y2 = q.y + q.y; float z2 = q.z + q.z;
    float xx = q.x * x2;  float xy = q.x * y2;  float xz = q.x * z2;
    float yy = q.y * y2;  float yz = q.y * z2;  float zz = q.z * z2;
    float wx = q.w * x2;  float wy = q.w * y2;  float wz = q.w * z2;

    float4x4 m = (float4x4)0;
    m[0][0] = 1.0f - (yy + zz); m[0][1] = xy + wz;          m[0][2] = xz - wy;          m[3][3] = 1.0f;
    m[1][0] = xy - wz;          m[1][1] = 1.0f - (xx + zz); m[1][2] = yz + wx;
    m[2][0] = xz + wy;          m[2][1] = yz - wx;          m[2][2] = 1.0f - (xx + yy);
    
    return m;
}

// ビルボード回転行列生成
float4x4 CalculateBillboardMatrix(float3 position, float3 cameraPos)
{
    float3 forward = normalize(cameraPos - position);
    float3 up = float3(0, 1, 0);
    // カメラが真上の場合
    if (abs(dot(forward, up)) > 0.99f) up = float3(0, 0, -1);
    
    float3 right = normalize(cross(up, forward));
    up = cross(forward, right);
    
    float4x4 m = (float4x4)0;
    m[0][0] = right.x;   m[0][1] = right.y;   m[0][2] = right.z;
    m[1][0] = up.x;      m[1][1] = up.y;      m[1][2] = up.z;
    m[2][0] = forward.x; m[2][1] = forward.y; m[2][2] = forward.z;
    m[3][3] = 1.0f;
    return m;
}

[numthreads(256, 1, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    uint id = dtid.x;
    if (id >= particleCount) return;

    Particle p = gParticles[id];
    
    // Aliveチェック
    if ((p.flags & 1) == 0)
    {
        gRenderParticles[id].World = (float4x4)0;
        gRenderParticles[id].WVP = (float4x4)0;
        gRenderParticles[id].color = float4(0,0,0,0);
        return;
    }

    // スケール行列
    float4x4 scaleMat = (float4x4)0;
    scaleMat[0][0] = p.scale.x;
    scaleMat[1][1] = p.scale.y;
    scaleMat[2][2] = p.scale.z;
    scaleMat[3][3] = 1.0f;

    // 回転行列
    float4x4 rotMat;
    if (isBillboard)
    {
        rotMat = CalculateBillboardMatrix(p.position, cameraEye);
        // Z回転のみ適用する？（通常ビルボードはZ回転許容）
        // ここでは簡易ビルボードとする
    }
    else
    {
        rotMat = QuaternionToMatrix(p.rotation);
    }

    // 平行移動行列
    float4x4 transMat = (float4x4)0;
    transMat[0][0] = 1.0f; transMat[1][1] = 1.0f; transMat[2][2] = 1.0f; transMat[3][3] = 1.0f;
    transMat[3][0] = p.position.x;
    transMat[3][1] = p.position.y;
    transMat[3][2] = p.position.z;

    // World行列合成 (Scale * Rot * Trans)
    float4x4 world = mul(scaleMat, mul(rotMat, transMat));

    // WVP行列
    float4x4 viewProj = mul(cameraView, cameraProjection);
    float4x4 wvp = mul(world, viewProj);

    // 出力
    gRenderParticles[id].World = world;
    gRenderParticles[id].WVP = wvp;
    gRenderParticles[id].color = p.color;
    gRenderParticles[id].uvOffsetScale = float4(0, 0, 1, 1); // TODO: Offset support
}
