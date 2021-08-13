# Direct3D 12 Mesh Demo

Render vertices using Direct3D 12-compatible GPUs.

## Interactions
- Xbox Controller
    |||
    |-|-|
    |A button|Toggle between wireframe and solid rendering mode|
    |D-pad|Translate camera|
    |LB/RB|Decrease/increase camera translation sensitivity|
    |LS/RS (rotate)|Rotate camera|
    |LS (press)|Reset camera position|
    |RS (press)|Reset camera direction|

- Keyboard
    |||
    |-|-|
    |Alt + Enter|Toggle between windowed/borderless and fullscreen mode|
    |Space|Toggle between wireframe and solid rendering mode|
    |W A S D ↑ ← ↓ →|Rotate camera|
    |Shift + W A S D ↑ ← ↓ →|Rotate camera at half speed|
    |PaUp/PgDn|Decrease/increase camera distance|
    |Home|Reset camera position and direction|
    |End|Reset camera position|

- Mouse
    |||
    |-|-|
    |Left button (drag)|Rotate camera|
    |Wheel|Decrease/increase camera distance|

---

## Library
```
// Defined in header "Meshes.h"

namespace Hydr10n {
    namespace Meshes {
        class MeshGenerator;
    }
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
std::vector<DirectX::VertexPositionNormalColor> m_vertices;
std::vector<uint32_t> m_indices;

void CreateSphere() {
    using namespace DirectX;
    using namespace Hydr10n::Meshes;

    MeshGenerator::VertexCollection vertices;
    MeshGenerator::IndexCollection indices;

    constexpr uint32_t SemiCircleSliceCount = 40;
    std::vector<XMFLOAT2> points;
    for (uint32_t i = 0; i <= SemiCircleSliceCount; i++) {
        const float radians = -XM_PIDIV2 + i * XM_PI / SemiCircleSliceCount;
        points.push_back({ cos(radians), sin(radians) });
    }

    MeshGenerator::CreateMeshAroundYAxis(vertices, indices, points.data(), points.size(), 1, SemiCircleSliceCount * 2, 0);

    m_vertices.clear();
    m_vertices.reserve(vertices.size());
    for (const auto& vertex : vertices)
        m_vertices.push_back({ vertex.position, vertex.normal,  XMFLOAT4(Colors::Green) });

    m_indices = std::move(indices);
}
```

https://user-images.githubusercontent.com/39995363/128587276-378d64d8-29ff-4679-97a1-c8777e8b92b1.mp4

## Example: Irregular Shape
```CPP
std::vector<DirectX::VertexPositionNormalColor> m_vertices;
std::vector<uint32_t> m_indices;

void CreateIrregularShape() {
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

    MeshGenerator::CreateMeshAroundYAxis(vertices, indices, Points, ARRAYSIZE(Points), 10, 30, 0);

    m_vertices.clear();
    for (const auto& vertex : vertices)
        m_vertices.push_back({ vertex.position, vertex.normal,  XMFLOAT4(Colors::Green) });

    m_indices = std::move(indices);
}
```

https://user-images.githubusercontent.com/39995363/128587277-0b1a21ff-0d04-424b-879e-de0b65437284.mp4
