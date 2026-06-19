// Skinning Compute Shader
// スキニング計算を行うコンピュートシェーダー

// SkinnedVertexData のバイトオフセット (C++と完全一致)
// Vector4 position:    offset 0,  size 16
// Vector2 texcoord:    offset 16, size 8
// Vector3 normal:      offset 24, size 12
// uint32_t boneIndices[4]: offset 36, size 16
// float boneWeights[4]:    offset 52, size 16
// Total: 68 bytes

static const uint SKINNED_VERTEX_SIZE = 68;
static const uint OFFSET_POSITION = 0;
static const uint OFFSET_TEXCOORD = 16;
static const uint OFFSET_NORMAL = 24;
static const uint OFFSET_BONE_INDICES = 36;
static const uint OFFSET_BONE_WEIGHTS = 52;

// 出力頂点構造体
struct OutputVertex
{
    float4 position;
    float2 texcoord;
    float3 normal;
};

// ボーン行列データ構造体（C++の行優先レイアウトと一致させる）
struct BoneMatrix
{
    row_major float4x4 matrix;
};

// ボーン行列バッファ
StructuredBuffer<BoneMatrix> gBoneMatrices : register(t0);

// 入力頂点バッファ (ByteAddressBuffer で明示的にバイト読み込み)
ByteAddressBuffer gInputVertices : register(t1);

// 出力頂点バッファ（UAV）
RWStructuredBuffer<OutputVertex> gOutputVertices : register(u0);

// 頂点数
cbuffer SkinningConstants : register(b0)
{
    uint gVertexCount;
    uint3 gPadding;
};

// 3x3行列の逆行列を計算
float3x3 Inverse3x3(float3x3 m)
{
    float det = determinant(m);
    
    // 特異行列の場合は単位行列を返す
    if (abs(det) < 1e-6)
    {
        return float3x3(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        );
    }
    
    float invDet = 1.0 / det;
    
    // 余因子行列（転置済み）
    float3x3 adj;
    adj[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * invDet;
    adj[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * invDet;
    adj[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invDet;
    adj[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invDet;
    adj[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invDet;
    adj[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * invDet;
    adj[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * invDet;
    adj[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * invDet;
    adj[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * invDet;
    
    return adj;
}

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint vertexIndex = DTid.x;
    
    // 範囲外チェック
    if (vertexIndex >= gVertexCount)
    {
        return;
    }
    
    // この頂点のベースオフセット
    uint baseOffset = vertexIndex * SKINNED_VERTEX_SIZE;
    
    // 各フィールドを明示的なオフセットで読み込み
    float4 position = asfloat(gInputVertices.Load4(baseOffset + OFFSET_POSITION));
    float2 texcoord = asfloat(gInputVertices.Load2(baseOffset + OFFSET_TEXCOORD));
    float3 normal = asfloat(gInputVertices.Load3(baseOffset + OFFSET_NORMAL));
    uint4 boneIndices = gInputVertices.Load4(baseOffset + OFFSET_BONE_INDICES);
    float4 boneWeights = asfloat(gInputVertices.Load4(baseOffset + OFFSET_BONE_WEIGHTS));
    
    // 位置スキニング行列
    float4x4 skinMatrix =
        gBoneMatrices[boneIndices.x].matrix * boneWeights.x +
        gBoneMatrices[boneIndices.y].matrix * boneWeights.y +
        gBoneMatrices[boneIndices.z].matrix * boneWeights.z +
        gBoneMatrices[boneIndices.w].matrix * boneWeights.w;

    // 位置変換
    float4 skinnedPosition = mul(position, skinMatrix);

    // 法線変換用の3x3行列を抽出
    float3x3 rotMatrix =
        (float3x3)gBoneMatrices[boneIndices.x].matrix * boneWeights.x +
        (float3x3)gBoneMatrices[boneIndices.y].matrix * boneWeights.y +
        (float3x3)gBoneMatrices[boneIndices.z].matrix * boneWeights.z +
        (float3x3)gBoneMatrices[boneIndices.w].matrix * boneWeights.w;

    // 逆転置行列を計算（法線変換の正しい方法）
    float3x3 normalMatrix = transpose(Inverse3x3(rotMatrix));

    // 法線を変換して正規化
    float3 skinnedNormal = normalize(mul(normal, normalMatrix));
    
    // 出力
    OutputVertex output;
    output.position = skinnedPosition;
    output.texcoord = texcoord;
    output.normal = skinnedNormal;
    
    gOutputVertices[vertexIndex] = output;
}
