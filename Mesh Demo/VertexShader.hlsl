#include "Shader.hlsli"

cbuffer cbPerObject : register(b0)
{
    float4x4 g_WorldViewProj;
};

struct VertexIn
{
    float3 Position : POSITION;
    float4 Color : COLOR;
};

[RootSignature(ROOT_SIGNATURE)]
Vertex main(VertexIn vertexIn)
{
    Vertex vertex;
    vertex.Position = mul(float4(vertexIn.Position, 1), g_WorldViewProj);
    vertex.Color = vertexIn.Color;
    return vertex;
}
