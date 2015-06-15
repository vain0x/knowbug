//formatted string

#ifndef IG_MODULE_STRF_H
#define IG_MODULE_STRF_H

#include <string>
#include <algorithm>

#include "../../cppformat/format.h"

//forwarder to adapt interface with hsp
template<typename... Args>
static std::string strf(char const* format, Args&&... args) {
	return fmt::sprintf(format, std::forward<Args>(args)...);
}

template<typename TIter>
std::string join(TIter&& begin, TIter&& end, char const* delimiter)
{
	std::stringstream ss;
	bool bFirst = true;
	std::for_each(std::forward<TIter>(begin), std::forward<TIter>(end), [&](decltype(*begin) const& val) {
		if ( bFirst ) { bFirst = false; } else { ss << delimiter; }
		ss << val;
	});
	return ss.str();
}

#endif
