#pragma once

#pragma warning(push)
#pragma warning(disable: 26812)

#include "pch.h"

#include "DeviceResources.h"
#include "GraphicsMemory.h"
#include "ResourceUploadBatch.h"
#include "BufferHelpers.h"

#include "StepTimer.h"

#include "CommonStates.h"
#include "EffectPipelineStateDescription.h"

#include "VertexTypes.h"
#include "Model.h"
#include "Meshes.h"

#include "DirectXHelpers.h"

#include "GamePad.h"
#include "Keyboard.h"
#include "Mouse.h"

#include "OrbitCamera.h"

#include <map>

#pragma warning(pop)

class D3DApp : public DX::IDeviceNotify {
public:
	D3DApp(const D3DApp&) = delete;
	D3DApp& operator=(const D3DApp&) = delete;

	D3DApp(HWND hWnd, const SIZE& outputSize) noexcept(false) {
		m_deviceResources->RegisterDeviceNotify(this);

		m_deviceResources->SetWindow(hWnd, static_cast<int>(outputSize.cx), static_cast<int>(outputSize.cy));

		m_deviceResources->CreateDeviceResources();
		CreateDeviceDependentResources();

		m_deviceResources->CreateWindowSizeDependentResources();
		CreateWindowSizeDependentResources();

		m_mouse->SetWindow(hWnd);

		m_orbitCamera.SetRadius(m_cameraRadius, MinCameraRadius, MaxCameraRadius);
	}

	~D3DApp() { m_deviceResources->WaitForGpu(); }

	SIZE GetOutputSize() const {
		const auto rc = m_deviceResources->GetOutputSize();
		return { rc.right - rc.left, rc.bottom - rc.top };
	}

	void Tick() {
		m_stepTimer.Tick([&] { Update(); });

		Render();
	}

	void OnWindowSizeChanged(const SIZE& outputSize) {
		if (!m_deviceResources->WindowSizeChanged(static_cast<int>(outputSize.cx), static_cast<int>(outputSize.cy))) return;

		CreateWindowSizeDependentResources();
	}

	void OnActivated() {}

	void OnDeactivated() {}

	void OnResuming() {
		m_stepTimer.ResetElapsedTime();

		m_gamepad->Resume();
	}

	void OnSuspending() { m_gamepad->Suspend(); }

	void OnDeviceLost() override {
		m_modelMeshPart.reset();

		m_basicEffects.clear();

		m_graphicsMemory.reset();
	}

	void OnDeviceRestored() override {
		CreateDeviceDependentResources();

		CreateWindowSizeDependentResources();
	}

private:
	using Vertex = DirectX::VertexPositionNormalColor;

	enum class RenderMode { Solid, Wireframe, Count };

	static constexpr float MinCameraRadius = 1, MaxCameraRadius = 10;

	const std::unique_ptr<DirectX::GamePad> m_gamepad = std::make_unique<decltype(m_gamepad)::element_type>();
	const std::unique_ptr<DirectX::Keyboard> m_keyboard = std::make_unique<decltype(m_keyboard)::element_type>();
	const std::unique_ptr<DirectX::Mouse> m_mouse = std::make_unique<decltype(m_mouse)::element_type>();

	std::unique_ptr<DX::DeviceResources> m_deviceResources = std::make_unique<decltype(m_deviceResources)::element_type>();
	std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;

	DX::StepTimer m_stepTimer;

	DirectX::GamePad::ButtonStateTracker m_gamepadButtonStateTrackers[DirectX::GamePad::MAX_PLAYER_COUNT];
	DirectX::Keyboard::KeyboardStateTracker m_keyboardStateTracker;
	DirectX::Mouse::ButtonStateTracker m_mouseButtonStateTracker;

	float m_cameraRadius = 3;
	DX::OrbitCamera m_orbitCamera;

	RenderMode m_renderMode = RenderMode::Solid;
	std::map<RenderMode, std::unique_ptr<DirectX::BasicEffect>> m_basicEffects;

	std::unique_ptr<DirectX::ModelMeshPart> m_modelMeshPart;

	void Clear() {
		const auto commandList = m_deviceResources->GetCommandList();

		PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

		const auto rtvDescriptor = m_deviceResources->GetRenderTargetView(), dsvDescriptor = m_deviceResources->GetDepthStencilView();
		commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
		commandList->ClearRenderTargetView(rtvDescriptor, DirectX::Colors::LightSteelBlue, 0, nullptr);
		commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);

		const auto viewport = m_deviceResources->GetScreenViewport();
		const auto scissorRect = m_deviceResources->GetScissorRect();
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		PIXEndEvent(commandList);
	}

	void Render() {
		if (!m_stepTimer.GetFrameCount()) return;

		m_deviceResources->Prepare();

		Clear();

		const auto commandList = m_deviceResources->GetCommandList();

		PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

		m_basicEffects[m_renderMode]->Apply(commandList);

		m_modelMeshPart->Draw(commandList);

		PIXEndEvent(commandList);

		PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");

		m_deviceResources->Present();

		m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

		PIXEndEvent();
	}

	void Update() {
		PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

		UpdateCamera();

		PIXEndEvent();
	}

	void CreateDeviceDependentResources() {
		m_graphicsMemory = std::make_unique<decltype(m_graphicsMemory)::element_type>(m_deviceResources->GetD3DDevice());

		CreateEffects();

		CreateMeshes();
	}

	void CreateWindowSizeDependentResources() {
		const auto size = GetOutputSize();
		m_orbitCamera.SetWindow(static_cast<int>(size.cx), static_cast<int>(size.cy));
	}

	void CreateEffects() {
		using namespace DirectX;

		const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

		for (int i = 0; i < static_cast<int>(RenderMode::Count); i++) {
			const auto renderMode = static_cast<RenderMode>(i);

			auto rasterizerDesc = CommonStates::CullNone;
			rasterizerDesc.FillMode = renderMode == RenderMode::Solid ? D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME;

			const EffectPipelineStateDescription psd(&Vertex::InputLayout, CommonStates::Opaque, CommonStates::DepthDefault, rasterizerDesc, rtState);

			auto& basicEffect = m_basicEffects[renderMode];
			basicEffect = std::make_unique<BasicEffect>(m_deviceResources->GetD3DDevice(), EffectFlags::VertexColor | EffectFlags::Lighting, psd);
			basicEffect->EnableDefaultLighting();
			basicEffect->DisableSpecular();
			basicEffect->SetDiffuseColor(Colors::White);
		}
	}

	void CreateMeshes() {
		using namespace DirectX;
		using namespace Hydr10n::Meshes;

		MeshGenerator::VertexCollection vertices;
		MeshGenerator::IndexCollection indices;

		constexpr auto SemiCircleSliceCount = 200;
		std::vector<XMFLOAT2> points;
		for (uint32_t i = 0; i <= SemiCircleSliceCount; i++) {
			const auto radians = -XM_PIDIV2 + i * XM_PI / SemiCircleSliceCount;
			points.push_back({ cos(radians), sin(radians) });
		}

		MeshGenerator::CreateMeshAroundYAxis(vertices, indices, points.data(), points.size(), 1, SemiCircleSliceCount * 2, 0);

		std::vector<Vertex> newVertices;
		newVertices.reserve(vertices.size());
		for (const auto& vertex : vertices) newVertices.push_back({ vertex.position, vertex.normal, XMFLOAT4(Colors::Teal) });

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer, indexBuffer;

		const auto device = m_deviceResources->GetD3DDevice();

		ResourceUploadBatch resourceUpload(device);
		resourceUpload.Begin();

		DX::ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, newVertices, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &vertexBuffer));
		DX::ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, indices, D3D12_RESOURCE_STATE_INDEX_BUFFER, &indexBuffer));

		resourceUpload.End(m_deviceResources->GetCommandQueue()).wait();

		m_modelMeshPart = std::make_unique<decltype(m_modelMeshPart)::element_type>(0);

		m_modelMeshPart->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		m_modelMeshPart->vertexStride = sizeof(newVertices[0]);
		m_modelMeshPart->vertexCount = static_cast<uint32_t>(newVertices.size());
		m_modelMeshPart->vertexBufferSize = static_cast<uint32_t>(m_modelMeshPart->vertexStride * newVertices.size());
		m_modelMeshPart->staticVertexBuffer = vertexBuffer;

		m_modelMeshPart->indexFormat = DXGI_FORMAT_R32_UINT;
		m_modelMeshPart->indexCount = static_cast<uint32_t>(indices.size());
		m_modelMeshPart->indexBufferSize = static_cast<uint32_t>(sizeof(indices[0]) * indices.size());
		m_modelMeshPart->staticIndexBuffer = indexBuffer;
	}

	void UpdateCamera() {
		using namespace DirectX;
		using Key = Keyboard::Keys;
		using GamepadButtonState = GamePad::ButtonStateTracker::ButtonState;
		using MouseButtonState = Mouse::ButtonStateTracker::ButtonState;

		const auto elapsedSeconds = static_cast<float>(m_stepTimer.GetElapsedSeconds()), totalSeconds = static_cast<float>(m_stepTimer.GetTotalSeconds());

		for (int i = 0; i < GamePad::MAX_PLAYER_COUNT; i++) {
			const auto gamePadState = m_gamepad->GetState(i);
			m_gamepadButtonStateTrackers[i].Update(gamePadState);

			if (gamePadState.IsConnected()) {
				if (gamePadState.thumbSticks.leftX || gamePadState.thumbSticks.leftY || gamePadState.thumbSticks.rightX || gamePadState.thumbSticks.rightY) m_mouse->SetVisible(false);

				if (m_gamepadButtonStateTrackers[i].a == GamepadButtonState::PRESSED) {
					m_mouse->SetVisible(false);

					m_renderMode = m_renderMode == RenderMode::Solid ? RenderMode::Wireframe : RenderMode::Solid;
				}

				m_orbitCamera.Update(elapsedSeconds * 2, gamePadState);
			}
		}

		const auto keyboardState = m_keyboard->GetState();
		m_keyboardStateTracker.Update(keyboardState);

		constexpr Key Keys[]{ Key::Space, Key::W, Key::A, Key::S, Key::D, Key::Up, Key::Left, Key::Down, Key::Right };
		for (const auto key : Keys) {
			if (m_keyboardStateTracker.IsKeyPressed(key)) {
				m_mouse->SetVisible(false);

				if (key == Key::Space) m_renderMode = m_renderMode == RenderMode::Solid ? RenderMode::Wireframe : RenderMode::Solid;
			}
		}

		const auto mouseState = m_mouse->GetState(), lastMouseState = m_mouseButtonStateTracker.GetLastState();
		m_mouseButtonStateTracker.Update(mouseState);

		if (m_mouseButtonStateTracker.leftButton == MouseButtonState::PRESSED) m_mouse->SetVisible(false);
		else if (m_mouseButtonStateTracker.leftButton == MouseButtonState::RELEASED || (m_mouseButtonStateTracker.leftButton == MouseButtonState::UP && (mouseState.x != lastMouseState.x || mouseState.y != lastMouseState.y))) m_mouse->SetVisible(true);

		if (mouseState.scrollWheelValue) {
			m_mouse->ResetScrollWheelValue();

			m_mouse->SetVisible(false);

			m_cameraRadius = std::clamp(m_cameraRadius - 0.2f * mouseState.scrollWheelValue / WHEEL_DELTA, MinCameraRadius, MaxCameraRadius);
			m_orbitCamera.SetRadius(m_cameraRadius, MinCameraRadius, MaxCameraRadius);
		}

		m_orbitCamera.Update(elapsedSeconds, *m_mouse, *m_keyboard);

		m_basicEffects[m_renderMode]->SetMatrices(XMMatrixIdentity(), m_orbitCamera.GetView(), m_orbitCamera.GetProjection());
	}
};
