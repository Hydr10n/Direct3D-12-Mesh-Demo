#pragma once

#include "BaseWindow.h"
#include "WindowHelpers.h"

#include "D3DApp.h"

#include "resource.h"

class MainWindow : public Hydr10n::Windows::BaseWindow {
public:
	MainWindow() noexcept(false) : BaseWindow(L"Direct3D 12", nullptr, 0, GetStockBrush(WHITE_BRUSH), LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON_DIRECTX))) {
		DX::ThrowIfFailed(Create(DefaultTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr));

		const auto window = GetWindow();

		RECT rc;
		DX::ThrowIfFailed(GetClientRect(window, &rc));
		const SIZE outputSize{ rc.right - rc.left, rc.bottom - rc.top };

		m_windowModeHelper = std::make_unique<decltype(m_windowModeHelper)::element_type>(window, outputSize, Hydr10n::WindowHelpers::WindowMode::Windowed, 0, WS_OVERLAPPEDWINDOW, FALSE);

		m_app = std::make_unique<decltype(m_app)::element_type>(window, outputSize);
	}

	WPARAM Run() {
		ShowWindow(GetWindow(), SW_SHOW);

		m_windowModeHelper->SetMode(m_windowModeHelper->GetMode());

		MSG msg;
		do
			if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
			else
				m_app->Tick();
		while (msg.message != WM_QUIT);
		return msg.wParam;
	}

private:
	static constexpr LPCWSTR DefaultTitle = L"Mesh";

	std::unique_ptr<Hydr10n::WindowHelpers::WindowModeHelper> m_windowModeHelper;

	std::unique_ptr<D3DApp> m_app;

	LRESULT CALLBACK OnMessageReceived(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		using namespace DirectX;

		switch (uMsg) {
		case WM_SYSKEYDOWN: {
			if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
				m_windowModeHelper->ToggleMode();
		} [[fallthrough]];
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
			Keyboard::ProcessMessage(uMsg, wParam, lParam);
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN: {
			SetCapture(hWnd);

			Mouse::ProcessMessage(uMsg, wParam, lParam);
		}	break;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
			ReleaseCapture();
			[[fallthrough]];
		case WM_INPUT:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHOVER:
			Mouse::ProcessMessage(uMsg, wParam, lParam);
			break;

		case WM_ACTIVATEAPP: {
			Keyboard::ProcessMessage(uMsg, wParam, lParam);
			Mouse::ProcessMessage(uMsg, wParam, lParam);

			if (wParam)
				m_app->OnActivated();
			else
				m_app->OnDeactivated();
		}	break;

		case WM_DPICHANGED: {
			const auto pRect = reinterpret_cast<PRECT>(lParam);
			SetWindowPos(hWnd, nullptr, static_cast<int>(pRect->left), static_cast<int>(pRect->top), static_cast<int>(pRect->right - pRect->left), static_cast<int>(pRect->bottom - pRect->top), SWP_NOZORDER);
		}	break;

		case WM_GETMINMAXINFO: {
			if (lParam)
				reinterpret_cast<PMINMAXINFO>(lParam)->ptMinTrackSize = { 320, 200 };
		}	break;

		case WM_MOVING:
		case WM_SIZING:
			m_app->Tick();
			break;

		case WM_SIZE: {
			switch (wParam) {
			case SIZE_MINIMIZED: m_app->OnSuspending(); break;
			case SIZE_RESTORED: m_app->OnResuming(); [[fallthrough]];
			default: m_app->OnWindowSizeChanged({ LOWORD(lParam), HIWORD(lParam) }); break;
			}
		}	break;

		case WM_MENUCHAR: return MAKELRESULT(0, MNC_CLOSE);

		case WM_DESTROY: PostQuitMessage(0); break;

		default: return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}

		return 0;
	}
};
