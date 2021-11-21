#pragma once

#include <Windows.h>

#include <system_error>

namespace ErrorHelpers {
	template <class T>
	inline void WINAPI throw_std_system_error(T code, const char* message = "") { throw std::system_error(static_cast<int>(code), std::system_category(), message); }

	inline void WINAPI ThrowIfFailed(BOOL val, LPCSTR lpMessage = "") { if (!val) throw_std_system_error(GetLastError(), lpMessage); }

	inline void WINAPI ThrowIfFailed(HRESULT val, LPCSTR lpMessage = "") { if (FAILED(val)) throw_std_system_error(val, lpMessage); }
}
