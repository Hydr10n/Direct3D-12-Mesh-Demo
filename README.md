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

## Library
```
// Defined in header "Meshes.h"
namespace Hydr10n {
    namespace Meshes {
        void CreateRingMesh(float minRadius, float maxRadius, float y, bool clockwise, uint32_t sliceCount, std::vector<DirectX::XMFLOAT3>& m_vertices, std::vector<uint32_t>& m_indices);

        void CreateMeshAroundYAxis(const DirectX::XMFLOAT2* pPoints, size_t pointCount, uint32_t stackCount, uint32_t sliceCount, float offsetX, std::vector<DirectX::XMFLOAT3>& m_vertices, std::vector<uint32_t>& m_indices);
    }
}
```

### Methods
|Name|Description|
|-|-|
|```CreateRingMesh```|Create 3D vertex and index data used to render a ring-shaped mesh paralleled with XZ-plane|
|```CreateMeshAroundYAxis```|Create 3D vertex and index data used to render a mesh revolved around Y-axis with given 2D adjacent vertices|

## Remarks
Current PSO in use may need to be created with D3D12_RASTERIZER_DESC::CullMode set to D3D12_CULL_MODE_NONE in order to render correctly.

## Example: Sphere
```CPP
std::vector<DirectX::VertexPositionColor> m_vertices;
std::vector<uint32_t> m_indices;

void CreateSphere() {
    using namespace DirectX;

    std::vector<XMFLOAT3> vertices;

    m_indices.clear();

    constexpr uint32_t SemiCircleSliceCount = 40;
    std::vector<XMFLOAT2> points;
    for (uint32_t i = 0; i <= SemiCircleSliceCount; i++) {
        const float radians = -XM_PIDIV2 + i * XM_PI / SemiCircleSliceCount;
        points.push_back({ cos(radians), sin(radians) });
    }

    Hydr10n::Meshes::CreateMeshAroundYAxis(points.data(), points.size(), 1, SemiCircleSliceCount * 2, 0, vertices, m_indices);

    m_vertices.clear();
    for (const auto& vertex : vertices)
        m_vertices.push_back({ vertex, XMFLOAT4(Colors::Green) });
}
```

https://user-images.githubusercontent.com/39995363/128587276-378d64d8-29ff-4679-97a1-c8777e8b92b1.mp4

## Example: Irregular Shape
```CPP
std::vector<DirectX::VertexPositionColor> m_vertices;
std::vector<uint32_t> m_indices;

void CreateIrregularShape() {
    std::vector<XMFLOAT3> vertices;

    m_indices.clear();

    constexpr XMFLOAT2 Points[]{
        {  0.0f, +1.0f },
        { +0.4f, +0.4f },
        { +1.0f,  0.0f },
        { +0.4f, -0.4f },
        {  0.0f, -1.0f }
    };

    Hydr10n::Meshes::CreateMeshAroundYAxis(Points, ARRAYSIZE(Points), 5, 30, 0, vertices, m_indices);
    
    m_vertices.clear();
    for (const auto& vertex : vertices)
        m_vertices.push_back({ vertex, XMFLOAT4(Colors::Green) });
}
```

https://user-images.githubusercontent.com/39995363/128587277-0b1a21ff-0d04-424b-879e-de0b65437284.mp4
