//formatted string

#pragma once
#include <string>

#include "../../cppformat/cppformat/format.h"

namespace {

// alias
template<typename... Args>
auto strf(char const* format, Args&&... args)
	-> std::string
{
	return fmt::sprintf(format, std::forward<Args>(args)...);
}

} //namespace
