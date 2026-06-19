// G-Buffer Pass Instanced Vertex Shader

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPos : TEXCOORD1;
};

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};

StructuredBuffer<TransformationMatrix> gInstanceMatrices : register(t0);

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    
    float4x4 worldMatrix = gInstanceMatrices[instanceId].World;
    float4x4 wvpMatrix = gInstanceMatrices[instanceId].WVP;
    float4x4 worldInverseTranspose = gInstanceMatrices[instanceId].WorldInverseTranspose;

    output.position = mul(input.position, wvpMatrix);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3)worldInverseTranspose));
    output.worldPos = mul(input.position, worldMatrix).xyz;
    
    return output;
}
