// string クラス支援モジュール

#include <cstdio>
#include <sstream>

#include "strf.h"

using std::string;

/// 書式付き文字列の生成
string vstrf(char const* format, va_list& arglist)
{
	static char stt_tmpbuf[0x2000];

	size_t const len = _vscprintf(format, arglist);

	if ( len < 0x2000 ) {
		vsprintf_s(stt_tmpbuf, format, arglist);
		return stt_tmpbuf;

	} else {
		size_t const size = len + 4;
		char* const buf = (char*)std::malloc(size * sizeof(char));

		vsprintf_s(buf, size, format, arglist);
		string const result = buf;

		std::free(buf);
		return std::move(result);
	}
}

string strf(char const* format, ...)
{
	va_list   arglist;
	va_start(arglist, format);

	string const result = vstrf(format, arglist);

	va_end(arglist);
	return std::move(result);
}

#if 0
/// 文字列の反復
string operator * ( string const& self, size_t n )
{
	string newOne = self.c_str();
	newOne *= n;
	return std::move(newOne);
}

string& operator *= (string& self, size_t n)
{
	string tmp = self;
	size_t const len = self.length();

	self.clear();
	self.reserve(len * n + 1);

	for ( size_t i = 0; i < n; ++i ) {
		self += tmp;
	}
	return self;
}
#endif
