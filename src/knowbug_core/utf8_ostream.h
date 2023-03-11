#pragma once

#include <iostream>
#include "encoding.h"

// UTF-8 エンコーディングの出力ストリーム。
// (標準出力に UTF-8 の文字列を流し込むためのものだと思ってよい。)
class Utf8OStream {
	std::ostream& out_;

public:
	Utf8OStream(std::ostream& out) : out_(out) {
	}

	auto operator <<(char const* str) & -> Utf8OStream& {
		out_ << str;
		return *this;
	}

	auto operator <<(std::string_view str) & -> Utf8OStream& {
		out_ << str;
		return *this;
	}

	// UTF-8 の文字列を流し込めるようにする:
	auto operator <<(char8_t const* str) & -> Utf8OStream& {
		return *this << (char const*)str;
	}

	auto operator <<(std::u8string_view str) & -> Utf8OStream& {
		return *this << std::string_view{ (char const*)str.data(), str.length() };
	}

	// 各種エンコーディングの文字列を流し込めるようにする:
	auto operator <<(HspStringView source) & -> Utf8OStream& {
		return *this << to_utf8(source);
	}

	auto operator <<(OsStringView source) & -> Utf8OStream& {
		return *this << to_utf8(source);
	}

	auto operator <<(SjisStringView source) & -> Utf8OStream& {
		return *this << to_utf8(source);
	}

	auto operator <<(std::size_t value) & -> Utf8OStream& {
		out_ << value;
		return *this;
	}

	// `<< std::endl` が動くようにする
	auto operator <<(std::ostream& (*fn)(std::ostream&)) & -> Utf8OStream& {
		out_ << fn;
		return *this;
	}

	// コピー・ムーブともに禁止。
	Utf8OStream(Utf8OStream const& other) = delete;
	Utf8OStream(Utf8OStream&& other) = delete;
	auto operator =(Utf8OStream const& other)->Utf8OStream & = delete;
	auto operator =(Utf8OStream&& other)->Utf8OStream & = delete;
};
