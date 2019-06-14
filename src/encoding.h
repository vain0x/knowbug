//! 文字列のエンコーディング関連

#pragma once

#include <cassert>
#include <cstring>
#include <string>
#include <Windows.h>
#include <tchar.h>

class OsString;
class OsStringView;
class SjisString;
class SjisStringView;
class Utf8String;
class Utf8StringView;

extern auto string_is_ascii(char const* str) -> bool;

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

// Windows API のための文字列への参照。
class OsStringView {
	LPCTSTR const inner_;
	mutable std::size_t size_;

public:
	explicit OsStringView(LPCTSTR inner, std::size_t size)
		: inner_(inner)
		, size_(size)
	{
		assert(inner != nullptr);
	}

	explicit OsStringView(LPCTSTR inner)
		: OsStringView(inner, std::size_t{ 0 })
	{
	}

	auto data() const -> LPCTSTR {
		return inner_;
	}

	auto size() const -> std::size_t {
		if (size_ == 0) {
			size_ = _tcslen(data());
		}
		return size_;
	}

	auto to_owned() const->OsString;

	auto to_hsp_string() const->HspString;

	auto to_sjis_string() const->SjisString;

	auto to_utf8_string() const->Utf8String;

	auto operator ==(OsStringView const& other) -> bool {
		return size() == other.size() && _tccmp(data(), other.data());
	}

	auto copy_to(LPTSTR dest, std::size_t dest_size) const {
		_tcsnccpy_s(dest, dest_size, data(), size());
	}
};

// Windows API のための文字列。
// UNICODE 版なら utf-16、そうでなければ ANSI (shift_jis)。
class OsString
	: public std::basic_string<TCHAR>
{
public:
	OsString() {}

	// FIXME: 暗黙のコピーはよくない。unordered_map に要求される。
	OsString(OsString const& other) : basic_string(other) {}

	auto operator =(OsString const& other) const->OsString& = delete;

	OsString(OsString&& other) : basic_string(other) {}

	explicit OsString(std::basic_string<TCHAR>&& inner) : basic_string(inner) {}

	static auto from_range(LPCTSTR begin, LPCTSTR end) -> OsString {
		assert(begin <= end);
		auto count = (std::size_t)(end - begin);
		return OsString{ std::basic_string<TCHAR>{ begin, count } };
	}

	auto operator =(OsString&& other) -> OsString& {
		swap(other);
		return *this;
	}

	auto as_ref() const -> OsStringView {
		return OsStringView{ data(), size() };
	}

	auto to_hsp_string() const->HspString;

	auto to_sjis_string() const->SjisString;

	auto to_utf8_string() const->Utf8String;
};

class SjisStringView {
	char const* const inner_;
	mutable std::size_t size_;

public:
	explicit SjisStringView(char const* inner, std::size_t size)
		: inner_(inner)
		, size_(size)
	{
		assert(inner != nullptr);
	}

	explicit SjisStringView(char const* inner)
		: SjisStringView(inner, std::size_t{ 0 })
	{
	}

	static auto from_ascii(char const* str) -> SjisStringView {
		assert(string_is_ascii(str));
		return SjisStringView{ str };
	}

	auto data() const -> char const* {
		return inner_;
	}

	auto size() const -> std::size_t {
		if (size_ == 0) {
			size_ = std::strlen(data());
		}
		return size_;
	}

	auto to_owned() const->SjisString;

	auto to_hsp_string() const->HspString;

	auto to_os_string() const->OsString;

	auto to_utf8_string() const->Utf8String;
};

// shift_jis (cp-932)
class SjisString
	: public std::string
{
public:
	SjisString() {}

	SjisString(SjisString&& other) : basic_string(other) {}

	explicit SjisString(std::string&& inner) : basic_string(inner) {}

	static auto from_ascii(char const* str) -> SjisString {
		assert(string_is_ascii(str));
		return SjisString{ std::string{str } };
	}

	auto as_ref() const -> SjisStringView {
		return SjisStringView{ data(), size() };
	}

	auto to_hsp_string() const->HspString;

	auto to_os_string() const->OsString;

	auto to_utf8_string() const->Utf8String;
};

// utf-8 エンコーディングの文字列への参照。
class Utf8StringView {
	char const* const inner_;
	mutable std::size_t size_;

public:
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

	static auto from_ascii(char const* str) -> Utf8StringView {
		assert(string_is_ascii(str));
		return Utf8StringView{ str, 0 };
	}

	auto data() const -> char const* {
		return inner_;
	}

	auto size() const -> std::size_t {
		if (size_ == 0) {
			size_ = std::strlen(data());
		}
		return size_;
	}

	auto to_owned() const->Utf8String;

	auto to_hsp_string() const->HspString;

	auto to_os_string() const->OsString;

	auto to_sjis_string() const->SjisString;
};

// utf-8 エンコーディングの文字列。
class Utf8String
	: public std::string
{
public:
	Utf8String() {}

	Utf8String(Utf8String&& other) : basic_string(other) {}

	explicit Utf8String(std::string&& inner) : basic_string(inner) {}

	static auto from_ascii(char const* str) -> Utf8String {
		assert(string_is_ascii(str));
		return Utf8String{ std::string{ str } };
	}

	auto as_ref() const -> Utf8StringView {
		return Utf8StringView{ data(), size() };
	}

	auto to_owned() const -> Utf8String {
		return as_ref().to_owned();
	}

	auto to_hsp_string() const -> HspString {
		return as_ref().to_hsp_string();
	}

	auto to_os_string() const -> OsString {
		return as_ref().to_os_string();
	}

	auto to_sjis_string() const -> SjisString {
		return as_ref().to_sjis_string();
	}
};

// utf-8 エンコーディングされた文字列リテラルをキャストする。
inline auto of_u8str(char const* str) -> Utf8StringView {
	return Utf8StringView{ str, 0 };
}

namespace std {
	template<>
	struct hash<OsString> {
		auto operator ()(OsString const& str) const -> std::size_t {
			return std::hash<std::basic_string<TCHAR>>{}(str);
		}
	};
}
