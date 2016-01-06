//formatted string

#pragma once
#include <string>
#include <algorithm>

#include "../cppformat/format.h"

namespace {

// alias
template<typename... Args>
auto strf(char const* format, Args&&... args)
	-> std::string
{
	return fmt::sprintf(format, std::forward<Args>(args)...);
}

template<typename TIter>
auto join(TIter&& begin, TIter&& end, char const* delimiter)
	-> std::string
{
	auto ss = std::stringstream {};
	auto bFirst = true;
	std::for_each(std::forward<TIter>(begin), std::forward<TIter>(end), [&](decltype(*begin) const& val) {
		if ( bFirst ) { bFirst = false; } else { ss << delimiter; }
		ss << val;
	});
	return ss.str();
}

} //namespace
