#pragma once

#include <Windows.h>
#include <windowsx.h>

namespace Windows {
	class BaseWindow {
	private:
		HWND m_hWnd{};
		WNDCLASSEXW m_wndClassEx;

	public:
		BaseWindow(const BaseWindow&) = delete;
		BaseWindow& operator=(const BaseWindow&) = delete;

		virtual ~BaseWindow() {
			if (m_hWnd != nullptr) {
				SetWindowLongPtrW(m_hWnd, GWLP_USERDATA, 0);

				DestroyWindow(m_hWnd);

				UnregisterClassW(m_wndClassEx.lpszClassName, m_wndClassEx.hInstance);
			}
		}

		HWND GetWindow() const { return m_hWnd; }

	protected:
		BaseWindow(const WNDCLASSEXW& wndClassEx) : m_wndClassEx(wndClassEx) {
			if (!m_wndClassEx.cbSize) m_wndClassEx.cbSize = sizeof(m_wndClassEx);

			if (m_wndClassEx.hInstance == nullptr) m_wndClassEx.hInstance = GetModuleHandle(nullptr);

			if (m_wndClassEx.hCursor == nullptr) m_wndClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);

			if (m_wndClassEx.hbrBackground == nullptr) m_wndClassEx.hbrBackground = GetStockBrush(WHITE_BRUSH);

			if (m_wndClassEx.lpfnWndProc == nullptr) m_wndClassEx.lpfnWndProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
				const auto _this = reinterpret_cast<decltype(this)>(uMsg == WM_NCCREATE ? reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams : reinterpret_cast<LPVOID>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)));
				if (_this != nullptr) return _this->OnMessageReceived(hWnd, uMsg, wParam, lParam);
				return DefWindowProcW(hWnd, uMsg, wParam, lParam);
			};

			if (m_wndClassEx.lpszClassName == nullptr) m_wndClassEx.lpszClassName = L"BaseWindow";
		}

		const WNDCLASSEXW& GetWindowClass() const { return m_wndClassEx; }

		BOOL Create(LPCWSTR lpWindowName, DWORD dwExStyle, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu = nullptr) {
			if (m_hWnd != nullptr) {
				SetLastError(ERROR_ALREADY_INITIALIZED);

				return FALSE;
			}

			if (!RegisterClassExW(&m_wndClassEx))
				return FALSE;

			if ((m_hWnd = CreateWindowExW(dwExStyle, m_wndClassEx.lpszClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, m_wndClassEx.hInstance, this)) == nullptr) {
				const auto lastError = GetLastError();

				UnregisterClassW(m_wndClassEx.lpszClassName, m_wndClassEx.hInstance);

				SetLastError(lastError);

				return FALSE;
			}

			SetWindowLongPtrW(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

			return TRUE;
		}

		BOOL Create(LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu = nullptr) { return Create(lpWindowName, 0, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu); }

		virtual LRESULT CALLBACK OnMessageReceived(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	};
};
