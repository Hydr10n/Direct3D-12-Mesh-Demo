# Direct3D 12 Mesh Demo

Render vertices using Direct3D 12-compatible GPUs.

## Controls
- Xbox Controller
    |||
    |-|-|
    |View button|Toggle between wireframe and solid render mode|
    |D-pad|Translate camera X/Y|
    |LS/RS (rotate)|Orbit camera X/Y|
    |LS (press)|Return camera to default focus/radius|
    |RS (press)|Reset camera|
    |LB/RB|Decrease/increase camera translation sensitivity|
    |LB + RB|Reset camera translation sensitivity|

- Keyboard
    |||
    |-|-|
    |Alt + Enter|Toggle between windowed/borderless and fullscreen mode|
    |Tab|Toggle between wireframe and solid render mode|
    |W A S D ↑ ← ↓ →|Translate camera X/Y|
    |Shift + W A S D ↑ ← ↓ →|Translate camera X/Y at half speed|
    |PageUp/PageDown|Translate camera Z|
    |Home|Reset camera|
    |End|Return camera to default focus/radius|

- Mouse
    |||
    |-|-|
    |Left button (drag)|Orbit camera X/Y|
    |Scroll wheel|Increase/decrease camera orbit radius|

---

## Library
```
// Defined in header "Meshes.h"

namespace Hydr10n::Meshes {
    struct MeshGenerator {
        using Vertex = DirectX::VertexPositionNormal;
        using VertexCollection = std::vector<Vertex>;
        using IndexCollection = std::vector<uint32_t>;

        static void CreateMeshAroundYAxis(
            VertexCollection& vertices, IndexCollection& indices,
            const DirectX::XMFLOAT2* pPoints, size_t pointCount,
            uint32_t verticalTessellation = 3, uint32_t horizontalTessellation = 3,
            float offsetX = 0
        );
    };
}
```

## Members

### Public Methods
|Name|Description|
|-|-|
|```CreateMeshAroundYAxis```|Create 3D vertex and index data used to render a mesh revolved around Y-axis with given 2D adjacent vertices|

## Remarks
Current PSO in use may need to be created with D3D12_RASTERIZER_DESC::CullMode set to D3D12_CULL_MODE_NONE in order to render correctly.

## Example: Sphere
```CPP
void CreateSphere() {
    using namespace DirectX;
    using namespace Hydr10n::Meshes;

    MeshGenerator::VertexCollection vertices;
    MeshGenerator::IndexCollection indices;

    constexpr auto SemiCircleSliceCount = 40;
    std::vector<XMFLOAT2> points;
    for (uint32_t i = 0; i <= SemiCircleSliceCount; i++) {
        const auto radians = -XM_PIDIV2 + XM_PI * static_cast<float>(i) / SemiCircleSliceCount;
        points.push_back({ cos(radians), sin(radians) });
    }

    MeshGenerator::CreateMeshAroundYAxis(vertices, indices, points.data(), points.size(), 1, SemiCircleSliceCount * 2);
}
```

https://user-images.githubusercontent.com/39995363/128587276-378d64d8-29ff-4679-97a1-c8777e8b92b1.mp4

## Example: Arbitrary Shape
```CPP
void CreateArbitraryShape() {
    using namespace DirectX;
    using namespace Hydr10n::Meshes;

    MeshGenerator::VertexCollection vertices;
    MeshGenerator::IndexCollection indices;

    constexpr XMFLOAT2 Points[]{
        {  0.0f, +1.0f },
        { +0.4f, +0.4f },
        { +1.0f,  0.0f },
        { +0.4f, -0.4f },
        {  0.0f, -1.0f }
    };

    MeshGenerator::CreateMeshAroundYAxis(vertices, indices, Points, ARRAYSIZE(Points), 10, 30);
}
```

https://user-images.githubusercontent.com/39995363/128587277-0b1a21ff-0d04-424b-879e-de0b65437284.mp4
