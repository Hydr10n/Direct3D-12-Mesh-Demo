#pragma once

#include <Windows.h>

namespace WindowHelpers {
	constexpr void CenterRect(_In_ const RECT& border, _Inout_ RECT& rect) {
		const auto rectWidth = rect.right - rect.left, rectHeight = rect.bottom - rect.top;
		rect.left = (border.right + border.left - rectWidth) / 2;
		rect.top = (border.bottom + border.top - rectHeight) / 2;
		rect.right = rect.left + rectWidth;
		rect.bottom = rect.top + rectHeight;
	}

	inline BOOL WINAPI CenterWindow(HWND hWnd) {
		const auto parent = GetParent(hWnd);
		if (parent != nullptr) {
			RECT border, rect;
			if (GetWindowRect(parent, &border) && GetWindowRect(hWnd, &rect)) {
				CenterRect(border, rect);

				return SetWindowPos(hWnd, nullptr, static_cast<int>(rect.left), static_cast<int>(rect.top), 0, 0, SWP_NOSIZE);
			}
		}

		return FALSE;
	}

	enum class WindowMode { Windowed, Borderless, Fullscreen };

	class WindowModeHelper {
	public:
		WindowModeHelper(HWND hMainWindow, const SIZE& outputSize, WindowMode mode, DWORD dwStyle, DWORD dwExStyle, BOOL bHasMenu) : m_hWnd(hMainWindow), m_outputSize(outputSize), m_currentMode(mode), m_lastMode(mode == WindowMode::Fullscreen ? WindowMode::Windowed : WindowMode::Fullscreen), m_Style(dwStyle), m_ExStyle(dwExStyle), m_HasMenu(bHasMenu) {}

		BOOL SetOutputSize(const SIZE& size) {
			if (m_currentMode == WindowMode::Fullscreen) {
				m_outputSize = size;

				SetLastError(ERROR_SUCCESS);

				return TRUE;
			}

			MONITORINFO monitorInfo;
			monitorInfo.cbSize = sizeof(monitorInfo);
			if (!GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &monitorInfo))
				return FALSE;

			RECT rc{ 0, 0, size.cx, size.cy };
			CenterRect(monitorInfo.rcMonitor, rc);
			if (!AdjustWindowRectEx(&rc, m_currentMode == WindowMode::Windowed ? m_Style : m_Style & ~WS_OVERLAPPEDWINDOW, m_HasMenu, m_ExStyle))
				return FALSE;

			const auto ret = SetWindowPos(m_hWnd, HWND_TOP, static_cast<int>(rc.left), static_cast<int>(rc.top), static_cast<int>(rc.right - rc.left), static_cast<int>(rc.bottom - rc.top), SWP_NOZORDER | SWP_FRAMECHANGED);
			if (ret)
				m_outputSize = size;

			return ret;
		}

		SIZE GetOutputSize() const { return m_outputSize; }

		BOOL SetMode(WindowMode mode) {
			const auto currentMode = m_currentMode;

			m_currentMode = mode;

			const auto ret = (SetWindowLongPtrW(m_hWnd, GWL_EXSTYLE, static_cast<LONG_PTR>(mode == WindowMode::Fullscreen ? m_ExStyle | WS_EX_TOPMOST : m_ExStyle)) || GetLastError() == ERROR_SUCCESS) && (SetWindowLongPtrW(m_hWnd, GWL_STYLE, static_cast<LONG_PTR>(mode == WindowMode::Windowed ? m_Style : m_Style & ~WS_OVERLAPPEDWINDOW)) || GetLastError() == ERROR_SUCCESS) && SetOutputSize(m_outputSize);
			if (ret) {
				ShowWindow(m_hWnd, mode == WindowMode::Fullscreen ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);

				m_lastMode = currentMode == mode ? m_lastMode : currentMode;
			}
			else
				m_currentMode = currentMode;

			return ret;
		}

		BOOL ToggleMode() { return SetMode(m_currentMode == WindowMode::Fullscreen ? m_lastMode : WindowMode::Fullscreen); }

		WindowMode GetMode() const { return m_currentMode; }

	private:
		const DWORD m_ExStyle, m_Style;

		const BOOL m_HasMenu;

		const HWND m_hWnd;

		WindowMode m_lastMode, m_currentMode;

		SIZE m_outputSize;
	};
}
