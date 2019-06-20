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
	LPCTSTR inner_;
	mutable std::size_t size_;

public:
	// FIXME: NULL 初期化はよくない。出力引数として使うときに要求されるので用意している。
	OsStringView()
		: inner_(nullptr)
		, size_(0)
	{
	}

	OsStringView(OsStringView&& other)
		: inner_(other.inner_)
		, size_(other.size_)
	{
	}

	OsStringView(OsStringView const& other)
		: inner_(other.inner_)
		, size_(other.size_)
	{
	}

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

	auto operator =(OsStringView&& other) -> OsStringView & {
		inner_ = other.inner_;
		size_ = other.size_;
		return *this;
	}

	auto operator =(OsStringView const& other) -> OsStringView & {
		inner_ = other.inner_;
		size_ = other.size_;
		return *this;
	}

	auto data() const -> LPCTSTR {
		if (inner_ == nullptr) {
			throw new std::exception{ "OsStringView is null" };
		}
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
		return size() == other.size() && _tccmp(data(), other.data()) == 0;
	}

	auto copy_to(LPTSTR dest, std::size_t dest_size) const -> void {
		_tcsnccpy_s(dest, dest_size, data(), size());
	}

	auto operator [](std::size_t i) const -> TCHAR {
		return i < size() ? data()[i] : TCHAR{};
	}

	auto begin() const -> LPCTSTR {
		return data();
	}

	auto end() const -> LPCTSTR {
		return data() + size();
	}
};

// Windows API のための文字列。
// UNICODE 版なら utf-16、そうでなければ ANSI (shift_jis)。
class OsString
	: public std::basic_string<TCHAR>
{
public:
	OsString() {}

	OsString(OsString&& other) : basic_string(other) {}

	// FIXME: 暗黙のコピーはよくない。map のキーとして使うときに要求されるので用意している。
	OsString(OsString const& other) : basic_string(other) {}

	auto operator =(OsString&& other)->OsString & {
		swap(other);
		return *this;
	}

	auto operator =(OsString const& other)->OsString & = delete;

	explicit OsString(std::basic_string<TCHAR>&& inner) : basic_string(inner) {}

	static auto from_range(LPCTSTR begin, LPCTSTR end) -> OsString {
		assert(begin <= end);
		auto count = (std::size_t)(end - begin);
		return OsString{ std::basic_string<TCHAR>{ begin, count } };
	}

	auto as_ref() const -> OsStringView {
		return OsStringView{ data(), size() };
	}

	auto to_owned() const -> OsString {
		return as_ref().to_owned();
	}

	auto to_hsp_string() const->HspString;

	auto to_sjis_string() const->SjisString;

	auto to_utf8_string() const->Utf8String;

	auto begin() const -> LPCTSTR {
		return as_ref().begin();
	}

	auto end() const -> LPCTSTR {
		return as_ref().end();
	}
};

class SjisStringView {
	char const* inner_;
	mutable std::size_t size_;

public:
	SjisStringView()
		: inner_{ nullptr }
		, size_{ 0 }
	{
	}

	SjisStringView(SjisStringView const& other)
		: inner_(other.inner_)
		, size_(other.size_)
	{
	}

	SjisStringView(SjisStringView&& other)
		: inner_(other.inner_)
		, size_(other.size_)
	{
	}

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

	auto operator =(SjisStringView&& other) -> SjisStringView & {
		inner_ = other.inner_;
		size_ = other.size_;
		return *this;
	}

	auto operator =(SjisStringView const& other) -> SjisStringView & {
		inner_ = other.inner_;
		size_ = other.size_;
		return *this;
	}

	static auto from_ascii(char const* str) -> SjisStringView {
		assert(string_is_ascii(str));
		return SjisStringView{ str };
	}

	auto data() const -> char const* {
		if (inner_ == nullptr) {
			throw new std::exception{ "SjisStringView is null" };
		}
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

	auto operator ==(SjisStringView const& other) const -> bool {
		return size() == other.size() && std::strcmp(data(), other.data()) == 0;
	}

	auto operator <(SjisStringView const& other) const -> bool {
		return std::strcmp(data(), other.data()) < 0;
	}

	auto begin() const -> char const* {
		return data();
	}

	auto end() const -> char const* {
		return data() + size();
	}
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

	auto operator =(SjisString&& other) -> SjisString & {
		swap(other);
		return *this;
	}

	auto as_ref() const -> SjisStringView {
		return SjisStringView{ data(), size() };
	}

	auto to_hsp_string() const->HspString;

	auto to_os_string() const->OsString;

	auto to_utf8_string() const->Utf8String;

	auto operator ==(SjisString const& other) const -> bool {
		return as_ref() == other.as_ref();
	}

	auto operator ==(SjisStringView const& other) const -> bool {
		return as_ref() == other;
	}

	auto operator <(SjisString const& other) const -> bool {
		return as_ref() < other.as_ref();
	}

	auto operator <(SjisStringView const& other) const -> bool {
		return as_ref() < other;
	}

	auto begin() const -> char const* {
		return as_ref().begin();
	}

	auto end() const -> char const* {
		return as_ref().end();
	}
};

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

	static auto from_ascii(char const* str) -> Utf8StringView {
		assert(string_is_ascii(str));
		return Utf8StringView{ str, 0 };
	}

	auto operator =(Utf8StringView&& other) -> Utf8StringView& {
		inner_ = other.inner_;
		size_ = other.size_;
		return *this;
	}

	auto data() const -> char const* {
		if (inner_ == nullptr) {
			throw new std::exception{ "Utf8StringView is null" };
		}
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

	static auto from_ascii(char const* str) -> Utf8String {
		assert(string_is_ascii(str));
		return Utf8String{ std::string{ str } };
	}

	auto operator =(Utf8String&& other) -> Utf8String & {
		swap(other);
		return *this;
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
