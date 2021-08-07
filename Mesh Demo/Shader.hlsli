#define ROOT_SIGNATURE "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)," \
    "CBV(b0, space = 0, visibility=SHADER_VISIBILITY_VERTEX)," \
    "CBV(b0, space = 0, visibility=SHADER_VISIBILITY_PIXEL)"

struct Vertex
{
    float4 Position : SV_Position, Color : COLOR;
};
