//! 文字列のエンコーディング関連

#pragma once

#include <cassert>
#include <cstring>
#include <string>
#include <string_view>
#include <Windows.h>
#include <tchar.h>

// shift_jis エンコーディングされた文字列の要素 (バイト)
enum class SjisChar
	: char
{
};

class Utf8String;
class Utf8StringView;

// Windows API のための文字列。
// UNICODE 版なら utf-16、そうでなければ ANSI (shift_jis)。
using OsString = std::basic_string<TCHAR>;

// Windows API のための文字列への参照。
using OsStringView = std::basic_string_view<TCHAR>;

// shift_jis (cp932) でエンコーディングされた文字列。
using SjisString = std::basic_string<SjisChar>;

// shift_jis 文字列への参照
using SjisStringView = std::basic_string_view<SjisChar>;

#ifdef HSP3_UTF8

// HSP が使用するエンコーディングの文字列。
using HspString = Utf8String;

// HSP が使用するエンコーディングの文字列への参照。
using HspStringView = Utf8StringView;

#else

// HSP が使用するエンコーディングの文字列。
using HspString = SjisString;

// HSP が使用するエンコーディングの文字列への参照。
using HspStringView = SjisStringView;

#endif

extern auto string_is_ascii(char const* str) -> bool;

extern auto ascii_as_utf8(char const* source) -> Utf8StringView;

extern auto ascii_to_utf8(std::string&& source) -> Utf8String;

extern auto ascii_to_utf8(std::string const& source) -> Utf8String;

extern auto as_hsp(char const* str) -> HspStringView;

extern auto to_hsp(OsStringView const& source) -> HspString;

extern auto to_hsp(SjisStringView const& source) -> HspString;

extern auto to_hsp(Utf8StringView const& source) -> HspString;

extern auto as_os(LPCTSTR str) -> OsStringView;

extern auto to_os(SjisStringView const& source) -> OsString;

extern auto to_os(Utf8StringView const& source) -> OsString;

extern auto as_sjis(char const* str) -> SjisStringView;

extern auto to_sjis(OsStringView const& source) -> SjisString;

extern auto to_sjis(Utf8StringView const& source) -> SjisString;

extern auto as_utf8(char const* str) -> Utf8StringView;

extern auto to_utf8(OsStringView const& source) -> Utf8String;

extern auto to_utf8(SjisStringView const& source) -> Utf8String;

extern auto to_owned(OsStringView const& source) -> OsString;

extern auto to_owned(SjisStringView const& source) -> SjisString;

extern auto to_owned(Utf8StringView const& source) -> Utf8String;

extern auto as_view(OsString const& source) -> OsStringView;

extern auto as_view(SjisString const& source) -> SjisStringView;

extern auto as_view(Utf8String const& source) -> Utf8StringView;

// utf-8 エンコーディングの文字列への参照。
class Utf8StringView {
	char const* inner_;
	mutable std::size_t size_;

public:
	Utf8StringView()
		: inner_{ nullptr }
		, size_{ 0 }
	{
	}

	Utf8StringView(Utf8StringView const& other)
		: inner_(other.inner_)
		, size_(other.size_)
	{
	}

	explicit Utf8StringView(char const* inner, std::size_t size)
		: inner_(inner)
		, size_(size)
	{
		assert(inner != nullptr);
	}

	explicit Utf8StringView(char const* inner)
		: Utf8StringView(inner, std::size_t{ 0 })
	{
	}

	auto operator =(Utf8StringView&& other) -> Utf8StringView& {
		inner_ = other.inner_;
		size_ = other.size_;
		return *this;
	}

	auto data() const -> char const* {
		if (inner_ == nullptr) {
			assert(false && u8"Utf8StringView is null");
			throw new std::exception{};
		}
		return inner_;
	}

	auto size() const -> std::size_t {
		if (size_ == 0) {
			size_ = std::strlen(data());
		}
		return size_;
	}

	auto operator ==(Utf8StringView const& other) const -> bool {
		return size() == other.size() && std::strcmp(data(), other.data()) == 0;
	}

	auto operator <(Utf8StringView const& other) const -> bool {
		return std::strcmp(data(), other.data()) < 0;
	}

	auto begin() const -> char const* {
		return data();
	}

	auto end() const -> char const* {
		return data() + size();
	}
};

// utf-8 エンコーディングの文字列。
class Utf8String
	: public std::string
{
public:
	Utf8String() {}

	Utf8String(Utf8String&& other) : basic_string(other) {}

	explicit Utf8String(std::string&& inner) : basic_string(inner) {}

	auto operator =(Utf8String&& other) -> Utf8String & {
		swap(other);
		return *this;
	}

	auto as_ref() const -> Utf8StringView {
		return Utf8StringView{ data(), size() };
	}

	auto operator ==(Utf8String const& other) const -> bool {
		return as_ref() == other.as_ref();
	}

	auto operator ==(Utf8StringView const& other) const -> bool {
		return as_ref() == other;
	}

	auto operator <(Utf8String const& other) const -> bool {
		return as_ref() < other.as_ref();
	}

	auto operator <(Utf8StringView const& other) const -> bool {
		return as_ref() < other;
	}

	auto begin() const -> char const* {
		return as_ref().begin();
	}

	auto end() const -> char const* {
		return as_ref().end();
	}
};

static auto to_hsp(OsString const& source) -> HspString {
	return to_hsp(as_view(source));
}

static auto to_hsp(SjisString const& source) -> HspString {
	return to_hsp(as_view(source));
}

static auto to_hsp(Utf8String const& source) -> HspString {
	return to_hsp(as_view(source));
}

static auto to_os(SjisString const& source) -> OsString {
	return to_os(as_view(source));
}

static auto to_os(Utf8String const& source) -> OsString {
	return to_os(as_view(source));
}

static auto to_sjis(OsString const& source) -> SjisString {
	return to_sjis(as_view(source));
}

static auto to_sjis(Utf8String const& source) -> SjisString {
	return to_sjis(as_view(source));
}

static auto to_utf8(OsString const& source) -> Utf8String {
	return to_utf8(as_view(source));
}

static auto to_utf8(SjisString const& source) -> Utf8String {
	return to_utf8(as_view(source));
}

namespace std {
	template<>
	struct hash<OsString> {
		auto operator ()(OsString const& str) const -> std::size_t {
			return std::hash<std::basic_string<TCHAR>>{}(str);
		}
	};
}
