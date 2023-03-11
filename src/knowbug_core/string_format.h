//formatted string

#pragma once
#include <string>

#include "../cppformat/cppformat/format.h"

namespace {

// 書式指定で文字列を作る。
template<typename... Args>
auto strf(char8_t const* format, Args&&... args)
	-> std::u8string
{
	return as_utf8(fmt::sprintf((char const *)format, std::forward<Args>(args)...));
}

} //namespace
