#pragma once

#include "WindowBase.h"
#include "WindowHelpers.h"

#include "D3DApp.h"

#include "resource.h"

class MainWindow : public Windows::WindowBase {
public:
	MainWindow() noexcept(false) :
		WindowBase(
			WNDCLASSEXW{
				.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_ICON_DIRECTX)),
				.lpszClassName = L"Direct3D 12"
			},
			DefaultTitle,
			0, WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			nullptr
		) {
		const auto hWnd = GetHandle();

		RECT rc;
		DX::ThrowIfFailed(GetClientRect(hWnd, &rc));
		const SIZE outputSize{ rc.right - rc.left, rc.bottom - rc.top };

		m_app = std::make_unique<decltype(m_app)::element_type>(hWnd, outputSize);

		m_windowModeHelper.Window = hWnd;
		m_windowModeHelper.ClientSize = outputSize;
	}

	WPARAM Run() {
		m_windowModeHelper.Apply();

		MSG msg;
		msg.message = WM_QUIT;
		do {
			if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
			else m_app->Tick();
		} while (msg.message != WM_QUIT);
		return msg.wParam;
	}

private:
	static constexpr LPCWSTR DefaultTitle = L"Mesh";

	WindowHelpers::WindowModeHelper m_windowModeHelper;

	std::unique_ptr<D3DApp> m_app;

	LRESULT CALLBACK OnMessageReceived(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		using namespace DirectX;

		switch (uMsg) {
		case WM_SYSKEYDOWN: {
			if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000) {
				m_windowModeHelper.ToggleMode();
				m_windowModeHelper.Apply();
			}
		} [[fallthrough]];
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP: Keyboard::ProcessMessage(uMsg, wParam, lParam); break;

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
		case WM_XBUTTONUP: ReleaseCapture(); [[fallthrough]];
		case WM_INPUT:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHOVER: Mouse::ProcessMessage(uMsg, wParam, lParam); break;

		case WM_ACTIVATEAPP: {
			Keyboard::ProcessMessage(uMsg, wParam, lParam);
			Mouse::ProcessMessage(uMsg, wParam, lParam);

			if (wParam) m_app->OnActivated();
			else m_app->OnDeactivated();
		}	break;

		case WM_DPICHANGED: {
			const auto rc = reinterpret_cast<PRECT>(lParam);
			SetWindowPos(hWnd, nullptr, static_cast<int>(rc->left), static_cast<int>(rc->top), static_cast<int>(rc->right - rc->left), static_cast<int>(rc->bottom - rc->top), SWP_NOZORDER);
		}	break;

		case WM_GETMINMAXINFO: if (lParam) reinterpret_cast<PMINMAXINFO>(lParam)->ptMinTrackSize = { 320, 200 }; break;

		case WM_MOVING:
		case WM_SIZING: m_app->Tick(); break;

		case WM_SIZE: {
			switch (wParam) {
			case SIZE_MINIMIZED: m_app->OnSuspending(); break;
			case SIZE_RESTORED: m_app->OnResuming(); [[fallthrough]];
			default: m_app->OnWindowSizeChanged(m_windowModeHelper.ClientSize = { LOWORD(lParam), HIWORD(lParam) }); break;
			}
		}	break;

		case WM_MENUCHAR: return MAKELRESULT(0, MNC_CLOSE);

		case WM_DESTROY: PostQuitMessage(ERROR_SUCCESS); break;

		default: return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}

		return 0;
	}
};
