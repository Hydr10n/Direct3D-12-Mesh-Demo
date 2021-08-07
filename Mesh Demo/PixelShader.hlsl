#include "Shader.hlsli"

[RootSignature(ROOT_SIGNATURE)]
float4 main(Vertex vertex) : SV_Target
{
	return vertex.Color;
}
