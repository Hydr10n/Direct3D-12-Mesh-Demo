/*
 * Header File: BaseWindow.h
 * Last Update: 2021/08/12
 *
 * Copyright (C) Hydr10n@GitHub. All Rights Reserved.
 */

#pragma once

#include <Windows.h>
#include <windowsx.h>

namespace Hydr10n {
	namespace Windows {
		class BaseWindow {
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

		private:
			HWND m_hWnd{};
			WNDCLASSEXW m_wndClassEx{ sizeof(m_wndClassEx) };

		protected:
			BaseWindow(LPCWSTR lpszClassName = L"BaseWindow", LPCWSTR lpszMenuName = nullptr, UINT style = 0, HBRUSH hbrBackground = GetStockBrush(WHITE_BRUSH), HICON hIcon = LoadIcon(nullptr, IDI_APPLICATION), HCURSOR hCursor = LoadCursor(nullptr, IDC_ARROW)) {
				m_wndClassEx.lpszClassName = lpszClassName;
				m_wndClassEx.lpszMenuName = lpszMenuName;
				m_wndClassEx.style = style;
				m_wndClassEx.hbrBackground = hbrBackground;
				m_wndClassEx.hIcon = hIcon;
				m_wndClassEx.hCursor = hCursor;
				m_wndClassEx.hInstance = GetModuleHandle(nullptr);
				m_wndClassEx.lpfnWndProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
					const auto _this = reinterpret_cast<decltype(this)>(uMsg == WM_NCCREATE ? reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams : reinterpret_cast<LPVOID>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)));
					if (_this != nullptr)
						return _this->OnMessageReceived(hWnd, uMsg, wParam, lParam);
					return DefWindowProcW(hWnd, uMsg, wParam, lParam);
				};
			}

			BaseWindow(const WNDCLASSEXW& wndClassEx) : m_wndClassEx(wndClassEx) {}

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
}