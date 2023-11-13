#include "pch.h"

#include "../knowbug_core/encoding.h"
#include "logger.h"

#ifdef _DEBUG

void debugf(char8_t const* format, ...) {
	va_list vlist1;
	va_start(vlist1, format);

	va_list vlist2;
	va_copy(vlist2, vlist1);

	char buf[512];
	auto len = vsnprintf_s(buf, sizeof(buf), (char const*)format, vlist1);
	assert(len >= 0);
	{
		auto os_str = to_os(std::u8string_view{(char8_t const*)buf, (std::size_t)len});
		os_str += _T("\n");
		OutputDebugString(os_str.data());
	}

	va_end(vlist1);
	va_end(vlist2);
}

#else

void debugf(char8_t const* format, ...) {
	// 何もしない
}

#endif
