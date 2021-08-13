/*
 * Header File: Meshes.h
 * Last Update: 2021/08/13
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include "VertexTypes.h"

#include <vector>
#include <map>
#include <set>

namespace Hydr10n {
	namespace Meshes {
		class MeshGenerator {
		public:
			using Vertex = DirectX::VertexPositionNormal;
			using VertexCollection = std::vector<Vertex>;
			using IndexCollection = std::vector<uint32_t>;

			static void CreateMeshAroundYAxis(VertexCollection& vertices, IndexCollection& indices, const DirectX::XMFLOAT2* pPoints, size_t pointCount, uint32_t verticalTessellation = 3, uint32_t horizontalTessellation = 3, float offsetX = 0) {
				using namespace DirectX;

				const auto radiansStep = XM_2PI / horizontalTessellation;

				auto minY = FLT_MAX;

				using Compare = decltype([](const XMFLOAT2& a, const XMFLOAT2& b) {
					if (a.x < b.x)
						return true;
					if (a.x > b.x)
						return false;
					return a.y < b.y;
					});
				std::map<XMFLOAT2, std::set<XMFLOAT2, Compare>, Compare> points;

				for (size_t i = 0; i < pointCount; i++) {
					const auto& a = pPoints[i], & b = pPoints[(i + 1) % pointCount];

					minY = min(minY, min(a.y, b.y));

					if ((i != pointCount - 1 || (a.x == b.x && a.y == b.y)) && (!points.contains(b) || !points[b].contains(a)))
						points[a].insert(b);
				}

				for (const auto& pair : points) {
					for (const auto& point : pair.second) {
						const auto baseIndex = static_cast<uint32_t>(vertices.size());

						const auto& bottom = (point.y < pair.first.y) ? point : pair.first, & top = (point.y >= pair.first.y) ? point : pair.first;

						if (top.y != bottom.y) {
							const auto height = top.y - bottom.y, tessellationHeight = height / verticalTessellation, radiusStep = (top.x - bottom.x) / verticalTessellation;

							for (uint32_t i = 0; i <= verticalTessellation; i++) {
								const auto radius = i * radiusStep + bottom.x + offsetX;

								for (uint32_t j = 0; j <= horizontalTessellation; j++) {
									const float c = cosf(j * radiansStep), s = sinf(j * radiansStep), dr = bottom.x - top.x;

									Vertex vertex;
									vertex.position = { radius * c, i * tessellationHeight + bottom.y, radius * s };
									XMStoreFloat3(&vertex.normal, XMVector3Normalize(XMVector3Cross({ -s, 0, c }, { dr * c, -height, dr * s })));
									vertices.push_back(std::move(vertex));
								}
							}

							const auto ringVertexCount = horizontalTessellation + 1;
							for (uint32_t i = 0; i < verticalTessellation; i++)
								for (uint32_t j = 0; j < horizontalTessellation; j++) {
									const auto a = i * ringVertexCount + j + baseIndex, b = (i + 1) * ringVertexCount + j + baseIndex, c = b + 1, d = a + 1;

									indices.push_back(a);
									indices.push_back(b);
									indices.push_back(c);

									indices.push_back(a);
									indices.push_back(c);
									indices.push_back(d);
								}
						}
						else
							CreateRing(vertices, indices, min(bottom.x, top.x) + offsetX, max(bottom.x, top.x) + offsetX, bottom.y, bottom.y > minY, horizontalTessellation);
					}
				}
			}

		private:
			static void CreateRing(VertexCollection& vertices, IndexCollection& indices, float innerRadius, float outerRadius, float y, bool clockwiseWinding, uint32_t tessellation) {
				using namespace DirectX;

				const XMFLOAT3 normal{ 0, clockwiseWinding ? 1.f : -1.f, 0 };

				const auto radiansStep = XM_2PI / tessellation;

				const auto baseIndex = static_cast<uint32_t>(vertices.size());

				for (uint32_t i = 0; i <= tessellation; i++) {
					const float x = outerRadius * cosf(i * radiansStep), z = outerRadius * sinf(i * radiansStep);
					vertices.push_back({ { x, y, z }, normal });
				}

				if (innerRadius) {
					for (uint32_t i = 0; i <= tessellation; i++) {
						const float x = innerRadius * cosf(i * radiansStep), z = innerRadius * sinf(i * radiansStep);
						vertices.push_back({ { x, y, z }, normal });
					}

					const auto ringVertexCount = tessellation + 1;
					for (uint32_t i = 0; i < tessellation; i++) {
						const auto a = i + baseIndex, b = ringVertexCount + i + baseIndex, c = b + 1, d = a + 1;

						indices.push_back(a);
						indices.push_back(clockwiseWinding ? b : d);
						indices.push_back(c);

						indices.push_back(a);
						indices.push_back(c);
						indices.push_back(clockwiseWinding ? d : b);
					}
				}
				else {
					vertices.push_back({ { 0, y, 0 }, normal });

					const auto centerIndex = static_cast<uint32_t>(vertices.size() - 1);
					for (uint32_t i = 0; i < tessellation; i++) {
						indices.push_back(centerIndex);
						indices.push_back(baseIndex + i + static_cast<uint32_t>(clockwiseWinding));
						indices.push_back(baseIndex + i + static_cast<uint32_t>(!clockwiseWinding));
					}
				}
			}
		};
	}
}
