// フォーマット付き文字列

// type unsafe
// use boost::format if able.

#ifndef IG_MODULE_STRF_H
#define IG_MODULE_STRF_H

#include <string>
#include <cstdarg>
#include <sstream>
#include <algorithm>

//template<typename ...TArgs> std::string strf_s(char const* format, TArgs&& ...args);

extern std::string  strf(char const* format, ...);
extern std::string vstrf(char const* format, va_list& arglist);

std::string  operator *  (std::string const& self, size_t n);
std::string& operator *= (std::string& self, size_t n);

// 列から文字列を生成する
template<typename TIter>
std::string join(TIter&& begin, TIter&& end, char const* delimiter)
{
	std::stringstream ss;
//	std::copy(std::forward<TIter>(begin), std::forward<TIter>(end),
//		std::ostream_iterator<string>(ss, delimiter));
	bool bFirst = true;
	std::for_each(std::forward<TIter>(begin), std::forward<TIter>(end), [&](decltype(*begin) const& val) {
		if ( bFirst ) { bFirst = false; } else { ss << delimiter; }
		ss << val;
	});
	return ss.str();
}

#endif
