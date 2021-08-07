/*
 * Header File: Meshes.h
 * Last Update: 2021/08/05
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include <DirectXMath.h>
#include <vector>
#include <map>
#include <set>

namespace Hydr10n {
	namespace Meshes {
		void CreateRingMesh(float minRadius, float maxRadius, float y, bool clockwise, uint32_t sliceCount, std::vector<DirectX::XMFLOAT3>& m_vertices, std::vector<uint32_t>& m_indices) {
			const auto radiansStep = DirectX::XM_2PI / sliceCount;

			const uint32_t baseIndex = static_cast<uint32_t>(m_vertices.size());

			for (uint32_t i = 0; i <= sliceCount; i++)
				m_vertices.push_back({ maxRadius * cosf(i * radiansStep), y, maxRadius * sinf(i * radiansStep) });

			if (minRadius) {
				for (uint32_t i = 0; i <= sliceCount; i++)
					m_vertices.push_back({ minRadius * cosf(i * radiansStep), y, minRadius * sinf(i * radiansStep) });

				const auto ringVertexCount = sliceCount + 1;
				for (uint32_t i = 0; i < sliceCount; i++) {
					const auto a = i + baseIndex, b = ringVertexCount + i + baseIndex, c = i + 1 + baseIndex, d = i + 1 + baseIndex;

					m_indices.push_back(a);
					m_indices.push_back(clockwise ? b : d);
					m_indices.push_back(c);

					m_indices.push_back(a);
					m_indices.push_back(c);
					m_indices.push_back(clockwise ? d : b);
				}
			}
			else {
				m_vertices.push_back({ 0, y, 0 });

				const uint32_t centerIndex = static_cast<uint32_t>(m_vertices.size() - 1);
				for (uint32_t i = 0; i < sliceCount; i++) {
					m_indices.push_back(centerIndex);
					m_indices.push_back(baseIndex + i + static_cast<uint32_t>(clockwise));
					m_indices.push_back(baseIndex + i + static_cast<uint32_t>(!clockwise));
				}
			}
		}

		void CreateMeshAroundYAxis(const DirectX::XMFLOAT2* pPoints, size_t pointCount, uint32_t stackCount, uint32_t sliceCount, float offsetX, std::vector<DirectX::XMFLOAT3>& m_vertices, std::vector<uint32_t>& m_indices) {
			using namespace std;
			using namespace DirectX;

			if (!pointCount || !stackCount || !sliceCount)
				return;

			const auto radiansStep = XM_2PI / sliceCount;

			float minY = FLT_MAX, maxY = -FLT_MAX;

			using Compare = decltype([](const XMFLOAT2& a, const XMFLOAT2& b) {
				if (a.x < b.x)
					return true;
				if (a.x > b.x)
					return false;
				return a.y < b.y;
				});
			map<XMFLOAT2, set<XMFLOAT2, Compare>, Compare> matrix;

			for (size_t i = 0; i < pointCount; i++) {
				const auto& a = pPoints[i], & b = pPoints[(i + 1) % pointCount];

				minY = min(minY, min(a.y, b.y));
				maxY = max(maxY, max(a.y, b.y));

				if (i != pointCount - 1 || (a.x == b.x && a.y == b.y))
					if (!matrix.contains(b) || !matrix[b].contains(a))
						matrix[a].insert(b);
			}

			for (const auto& points : matrix) {
				for (const auto& point : points.second) {
					const uint32_t baseIndex = static_cast<uint32_t>(m_vertices.size());

					const auto& bottom = (point.y < points.first.y) ? point : points.first, & top = (point.y >= points.first.y) ? point : points.first;

					if (top.y != bottom.y) {
						const auto height = top.y - bottom.y, stackHeight = height / stackCount, radiusStep = (top.x - bottom.x) / stackCount;

						for (uint32_t i = 0; i <= stackCount; i++) {
							const auto radius = i * radiusStep + bottom.x + offsetX;

							for (uint32_t j = 0; j <= sliceCount; j++)
								m_vertices.push_back({ radius * cosf(j * radiansStep), i * stackHeight + bottom.y, radius * sinf(j * radiansStep) });
						}

						const auto ringVertexCount = sliceCount + 1;
						for (uint32_t i = 0; i < stackCount; i++)
							for (uint32_t j = 0; j < sliceCount; j++) {
								const auto a = i * ringVertexCount + j + baseIndex, b = (i + 1) * ringVertexCount + j + baseIndex, c = (i + 1) * ringVertexCount + j + 1 + baseIndex, d = i * ringVertexCount + j + 1 + baseIndex;

								m_indices.push_back(a);
								m_indices.push_back(b);
								m_indices.push_back(c);

								m_indices.push_back(a);
								m_indices.push_back(c);
								m_indices.push_back(d);
							}
					}
					else if (bottom.y <= minY || top.y >= maxY)
						CreateRingMesh(min(bottom.x, top.x) + offsetX, max(bottom.x, top.x) + offsetX, bottom.y, top.y >= maxY, sliceCount, m_vertices, m_indices);
				}
			}
		}
	}
}
