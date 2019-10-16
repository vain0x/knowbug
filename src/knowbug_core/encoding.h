//! 文字列のエンコーディング関連

#pragma once

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include "platform.h"

// HSP のランタイムが使うエンコーディングの文字列の要素 (バイト)
enum class HspChar
	: char
{
};

// shift_jis エンコーディングされた文字列の要素 (バイト)
enum class SjisChar
	: char
{
};

// UTF-8 エンコーディングされた文字列の要素 (バイト)
// NOTE: char8_t が存在しなかった時代のため
enum class Utf8Char
	: char
{
};

// HSP ランタイムが使うエンコーディングの文字列。
using HspString = std::basic_string<HspChar>;

// HSP ランタイムが使うエンコーディングの文字列への参照。
using HspStringView = std::basic_string_view<HspChar>;

// Windows API のための文字列。
// UNICODE 版なら utf-16、そうでなければ ANSI (shift_jis)。
using OsString = std::basic_string<TCHAR>;

// Windows API のための文字列への参照。
using OsStringView = std::basic_string_view<TCHAR>;

// shift_jis (cp932) でエンコーディングされた文字列。
using SjisString = std::basic_string<SjisChar>;

// shift_jis 文字列への参照
using SjisStringView = std::basic_string_view<SjisChar>;

// UTF-8 でエンコーディングされた文字列
using Utf8String = std::basic_string<Utf8Char>;

// UTF-8 文字列への参照
using Utf8StringView = std::basic_string_view<Utf8Char>;

extern auto ascii_as_utf8(char const* source) -> Utf8StringView;

extern auto ascii_as_utf8(std::string&& source) -> Utf8String;

extern auto ascii_to_utf8(std::string const& source) -> Utf8String;

extern auto as_hsp(char const* str) -> HspStringView;

extern auto as_hsp(std::string_view str) -> HspStringView;

extern auto as_hsp(std::string&& str) -> HspString;

extern auto to_hsp(OsStringView source) -> HspString;

extern auto to_hsp(SjisStringView source) -> HspString;

extern auto to_hsp(Utf8StringView source) -> HspString;

extern auto to_os(HspStringView source) -> OsString;

extern auto to_os(SjisStringView source) -> OsString;

extern auto to_os(Utf8StringView source) -> OsString;

extern auto as_sjis(char const* str) -> SjisStringView;

extern auto as_sjis(std::string_view str) -> SjisStringView;

extern auto as_sjis(std::string&& source) -> SjisString;

extern auto to_sjis(HspStringView source) -> SjisString;

extern auto to_sjis(OsStringView source) -> SjisString;

extern auto to_sjis(Utf8StringView source) -> SjisString;

extern auto as_utf8(char const* str) -> Utf8StringView;

extern auto as_utf8(std::string_view str) -> Utf8StringView;

extern auto as_utf8(std::string&& source) -> Utf8String;

extern auto to_utf8(HspStringView source) -> Utf8String;

extern auto to_utf8(OsStringView source) -> Utf8String;

extern auto to_utf8(SjisStringView source) -> Utf8String;

static auto to_owned(HspStringView source) -> HspString {
	return HspString{ source };
}

static auto to_owned(OsStringView source) -> OsString {
	return OsString{ source };
}

static auto to_owned(SjisStringView source) -> SjisString {
	return SjisString{ source };
}

static auto to_owned(Utf8StringView source) -> Utf8String {
	return Utf8String{ source };
}

static auto as_native(HspStringView source) -> std::string_view {
	return std::string_view{ (std::string_view const&)source };
}

static auto as_native(HspString&& source) -> std::string {
	return std::string{ (std::string&&)source };
}

static auto as_native(SjisStringView source) -> std::string_view {
	return std::string_view{ (std::string_view const&)source };
}

static auto as_native(SjisString&& source) -> std::string {
	return std::string{ (std::string&&)source };
}

static auto as_native(Utf8StringView source) -> std::string_view {
	return std::string_view{ (std::string_view const&)source };
}

static auto as_native(Utf8String&& source) -> std::string {
	return std::string{ (std::string&&)source };
}

static std::ostream& operator <<(std::ostream& out, Utf8StringView source) {
	return out << as_native(source);
}

static std::ostream& operator <<(std::ostream& out, HspStringView source) {
	return out << to_utf8(source);
}

static std::ostream& operator <<(std::ostream& out, OsStringView source) {
	return out << to_utf8(source);
}

static std::ostream& operator <<(std::ostream& out, SjisStringView source) {
	return out << to_utf8(source);
}
