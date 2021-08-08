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

#include "ReadData.h"

#include "VertexTypes.h"
#include "Model.h"

#include "DirectXHelpers.h"

#include "GamePad.h"
#include "Keyboard.h"
#include "Mouse.h"

#include "OrbitCamera.h"

#include "Meshes.h"

#include <filesystem>

#pragma warning(pop)

class D3DApp : public DX::IDeviceNotify {
public:
	D3DApp(const D3DApp&) = delete;
	D3DApp& operator=(const D3DApp&) = delete;

	D3DApp(HWND hWnd, UINT width, UINT height, double targetFPS, bool isFixedTimeStep) noexcept(false) {
		LoadShaders();
		CreateMeshes();

		m_deviceResources->RegisterDeviceNotify(this);

		m_deviceResources->SetWindow(hWnd, static_cast<int>(width), static_cast<int>(height));

		m_deviceResources->CreateDeviceResources();
		CreateDeviceDependentResources();

		m_deviceResources->CreateWindowSizeDependentResources();
		CreateWindowSizeDependentResources();

		m_stepTimer.SetTargetElapsedSeconds(1 / targetFPS);
		m_stepTimer.SetFixedTimeStep(isFixedTimeStep);

		m_mouse->SetWindow(hWnd);

		m_orbitCamera.SetRadius(DefaultCameraRadius, MinCameraRadius, MaxCameraRadius);
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

	void OnWindowSizeChanged(WPARAM wParam, LPARAM lParam) {
		if (!m_deviceResources->WindowSizeChanged(LOWORD(lParam), HIWORD(lParam)))
			return;

		CreateWindowSizeDependentResources();
	}

	void OnActivated() {}

	void OnDeactivated() {}

	void OnResuming() {
		m_stepTimer.ResetElapsedTime();

		m_gamepad->Resume();
	}

	void OnSuspending() {
		m_gamepad->Suspend();
	}

	void OnDeviceLost() override {
		m_modelMeshPart.reset();

		m_wireframePSO.Reset();
		m_solidPSO.Reset();

		m_rootSignature.Reset();

		m_graphicsMemory.reset();
	}

	void OnDeviceRestored() override {
		CreateDeviceDependentResources();

		CreateWindowSizeDependentResources();
	}

private:
	struct ConstantBufferParams { DirectX::XMFLOAT4X4 WorldViewProj; };

	static constexpr float DefaultCameraRadius = 3, MinCameraRadius = 1, MaxCameraRadius = 10;

	const DirectX::XMVECTORF32 DefaultBackgroundColor = DirectX::Colors::LightSteelBlue;

	const std::unique_ptr<DirectX::GamePad> m_gamepad = std::make_unique<decltype(m_gamepad)::element_type>();
	const std::unique_ptr<DirectX::Keyboard> m_keyboard = std::make_unique<decltype(m_keyboard)::element_type>();
	const std::unique_ptr<DirectX::Mouse> m_mouse = std::make_unique<decltype(m_mouse)::element_type>();

	const std::unique_ptr<DX::DeviceResources> m_deviceResources = std::make_unique<decltype(m_deviceResources)::element_type>();

	std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;

	DX::StepTimer m_stepTimer;

	bool m_enableWireframe = true;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_wireframePSO, m_solidPSO;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

	ConstantBufferParams m_constantBufferParams;

	std::vector<uint8_t> m_vertexShaderData, m_pixelShaderData;

	std::vector<DirectX::VertexPositionColor> m_vertices;
	std::vector<uint32_t> m_indices;

	std::unique_ptr<DirectX::ModelMeshPart> m_modelMeshPart;

	DirectX::Keyboard::KeyboardStateTracker m_keyboardStateTracker;
	DirectX::Mouse::ButtonStateTracker m_mouseButtonStatetracker;
	DirectX::GamePad::ButtonStateTracker m_gamepadButtonStateTrackers[DirectX::GamePad::MAX_PLAYER_COUNT];

	float m_cameraRadius = DefaultCameraRadius;
	DX::OrbitCamera m_orbitCamera;

	void Clear() {
		const auto commandList = m_deviceResources->GetCommandList();

		PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

		const auto rtvDescriptor = m_deviceResources->GetRenderTargetView(), dsvDescriptor = m_deviceResources->GetDepthStencilView();
		commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
		commandList->ClearRenderTargetView(rtvDescriptor, DefaultBackgroundColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);

		const auto viewport = m_deviceResources->GetScreenViewport();
		const auto scissorRect = m_deviceResources->GetScissorRect();
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);

		PIXEndEvent(commandList);
	}

	void Render() {
		if (!m_stepTimer.GetFrameCount())
			return;

		m_deviceResources->Prepare();
		Clear();

		const auto commandList = m_deviceResources->GetCommandList();

		PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

		commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		commandList->SetGraphicsRootConstantBufferView(0, m_graphicsMemory->AllocateConstant(m_constantBufferParams.WorldViewProj).GpuAddress());

		commandList->SetPipelineState(m_enableWireframe ? m_wireframePSO.Get() : m_solidPSO.Get());

		m_modelMeshPart->Draw(commandList);

		PIXEndEvent(commandList);

		PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");

		m_deviceResources->Present();

		m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

		PIXEndEvent();
	}

	void Update() {
		using namespace DirectX;
		using Key = Keyboard::Keys;
		using KeyboardState = DirectX::Keyboard::State;
		using MouseButtonState = DirectX::Mouse::ButtonStateTracker::ButtonState;
		using GamepadButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

		const auto elapsledSeconds = static_cast<float>(m_stepTimer.GetElapsedSeconds());

		PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

		for (int i = 0; i < GamePad::MAX_PLAYER_COUNT; i++) {
			const auto gamePadState = m_gamepad->GetState(i);
			m_gamepadButtonStateTrackers[i].Update(gamePadState);

			if (gamePadState.IsConnected()) {
				if (gamePadState.thumbSticks.leftX || gamePadState.thumbSticks.leftY || gamePadState.thumbSticks.rightX || gamePadState.thumbSticks.rightY)
					m_mouse->SetVisible(false);

				if (m_gamepadButtonStateTrackers[i].a == GamepadButtonState::PRESSED) {
					m_enableWireframe = !m_enableWireframe;

					m_mouse->SetVisible(false);
				}

				m_orbitCamera.Update(elapsledSeconds * 2, gamePadState);
			}
		}

		const auto keyboardState = m_keyboard->GetState();
		m_keyboardStateTracker.Update(keyboardState);

		constexpr Key Keys[]{ Key::Space, Key::W, Key::A, Key::S, Key::D, Key::Up, Key::Left, Key::Down, Key::Right };
		for (const auto key : Keys)
			if (m_keyboardStateTracker.IsKeyPressed(key)) {
				m_mouse->SetVisible(false);

				if (key == Key::Space)
					m_enableWireframe = !m_enableWireframe;
			}

		const auto mouseState = m_mouse->GetState(), lastMouseState = m_mouseButtonStatetracker.GetLastState();
		m_mouseButtonStatetracker.Update(mouseState);

		if (m_mouseButtonStatetracker.leftButton == MouseButtonState::PRESSED)
			m_mouse->SetVisible(false);
		else if (m_mouseButtonStatetracker.leftButton == MouseButtonState::RELEASED || (m_mouseButtonStatetracker.leftButton == MouseButtonState::UP && (mouseState.x != lastMouseState.x || mouseState.y != lastMouseState.y)))
			m_mouse->SetVisible(true);

		if (mouseState.scrollWheelValue) {
			m_mouse->ResetScrollWheelValue();

			m_mouse->SetVisible(false);

			m_cameraRadius = std::clamp(m_cameraRadius - 0.1f * mouseState.scrollWheelValue / WHEEL_DELTA, MinCameraRadius, MaxCameraRadius);
			m_orbitCamera.SetRadius(m_cameraRadius, MinCameraRadius, MaxCameraRadius);
		}

		m_orbitCamera.Update(elapsledSeconds, *m_mouse, *m_keyboard.get());

		XMStoreFloat4x4(&m_constantBufferParams.WorldViewProj, XMMatrixTranspose(XMMatrixIdentity() * m_orbitCamera.GetView() * m_orbitCamera.GetProjection()));

		PIXEndEvent();
	}

	void CreateDeviceDependentResources() {
		using namespace DirectX;

		const auto device = m_deviceResources->GetD3DDevice();

		m_graphicsMemory = std::make_unique<decltype(m_graphicsMemory)::element_type>(device);

		CD3DX12_ROOT_PARAMETER rootParameter;
		rootParameter.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		const CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(1, &rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		DX::ThrowIfFailed(CreateRootSignature(device, &rootSignatureDesc, m_rootSignature.GetAddressOf()));

		static constexpr D3D12_INPUT_ELEMENT_DESC InputElementDesc[]{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA }
		};
		constexpr D3D12_INPUT_LAYOUT_DESC InputLayoutDesc{ InputElementDesc, ARRAYSIZE(InputElementDesc) };
		for (int i = 0; i < 2; i++) {
			auto rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			rasterizerDesc.FillMode = i ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
			rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
			EffectPipelineStateDescription(&InputLayoutDesc, CommonStates::Opaque, CommonStates::DepthDefault, rasterizerDesc, RenderTargetState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat())).CreatePipelineState(device, m_rootSignature.Get(), { reinterpret_cast<PVOID>(m_vertexShaderData.data()), static_cast<SIZE_T>(m_vertexShaderData.size()) }, { reinterpret_cast<PVOID>(m_pixelShaderData.data()), static_cast<SIZE_T>(m_pixelShaderData.size()) }, i ? m_wireframePSO.ReleaseAndGetAddressOf() : m_solidPSO.ReleaseAndGetAddressOf());
		}

		m_modelMeshPart = std::make_unique<decltype(m_modelMeshPart)::element_type>(0);
		m_modelMeshPart->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		m_modelMeshPart->vertexStride = sizeof(m_vertices[0]);
		m_modelMeshPart->vertexCount = static_cast<uint32_t>(m_vertices.size());
		m_modelMeshPart->vertexBufferSize = sizeof(m_vertices[0]) * m_modelMeshPart->vertexCount;
		m_modelMeshPart->indexFormat = DXGI_FORMAT_R32_UINT;
		m_modelMeshPart->indexCount = static_cast<uint32_t>(m_indices.size());
		m_modelMeshPart->indexBufferSize = sizeof(m_indices[0]) * m_modelMeshPart->indexCount;
		ResourceUploadBatch resourceUpload(device);
		resourceUpload.Begin();
		DX::ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, m_vertices, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_modelMeshPart->staticVertexBuffer.ReleaseAndGetAddressOf()));
		DX::ThrowIfFailed(CreateStaticBuffer(device, resourceUpload, m_indices, D3D12_RESOURCE_STATE_INDEX_BUFFER, m_modelMeshPart->staticIndexBuffer.ReleaseAndGetAddressOf()));
		resourceUpload.End(m_deviceResources->GetCommandQueue()).wait();
	}

	void CreateWindowSizeDependentResources() {
		const auto size = GetOutputSize();
		m_orbitCamera.SetWindow(static_cast<int>(size.cx), static_cast<int>(size.cy));
	}

	void LoadShaders() {
		const auto path = std::filesystem::path(*__wargv).replace_filename(L"Shaders\\").native();
		m_vertexShaderData = DX::ReadData((path + L"VertexShader.cso").c_str());
		m_pixelShaderData = DX::ReadData((path + L"PixelShader.cso").c_str());
	}

	void CreateMeshes() {
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
};
