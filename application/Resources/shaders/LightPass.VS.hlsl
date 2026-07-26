// Light Pass Vertex Shader
// フルスクリーンクワッド用頂点シェーダー

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// 頂点データなしでフルスクリーンクワッドを描画
VertexShaderOutput main(uint vertexID : SV_VertexID)
{
    VertexShaderOutput output;
    
    // 頂点ID 0,1,2,3 でフルスクリーンクワッドを生成
    // 0: (-1, -1), 1: (1, -1), 2: (-1, 1), 3: (1, 1)
    float2 pos = float2((vertexID & 1) * 2.0f - 1.0f, (vertexID >> 1) * 2.0f - 1.0f);
    
    output.position = float4(pos.x, -pos.y, 0.0f, 1.0f);
    output.texcoord = float2((vertexID & 1), (vertexID >> 1));
    
    return output;
}
