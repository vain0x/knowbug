// 文字列のエンコーディング関連
//
// 目的: 文字列のエンコーディングに関する不具合を防ぐための抽象化を提供する。
//
// 方法: 文字列エンコーディングが異なる可能性がある文字列は、異なる型で扱う。
//
// ## 動機
//
// 文字列を `char const*` や `std::string` (= `std::basic_string<char>`) で表すと、
// エンコーディングが shift_jis なのか UTF-8 なのか区別がつかない。
// もし shift_jis 文字列と UTF-8 文字列を連結してしまったり、
// shift_jis 文字列を UTF-8 と誤認してエンコーディング変換にかけたりすると、データが壊れてしまう。
//
// ## 詳細
//
// HspChar, Utf8Char などの「文字型」を用意する。
// これらは実行時には char と同じだが、型システム的には char とは区別される。
//
// 標準ライブラリに `std::basic_string<Char>` という型テンプレートがある。
// これは各文字が型 Char によって表されるような「文字列」を表す。
//
// ここでは、型 Char によって文字列のエンコーディングが決まることにする。
// 例えば Char=Utf8Char のときの `std::basic_string<Utf8Char>` は、
// UTF-8 エンコーディングされた文字列である。
//
// これにより、文字列エンコーディングに関する誤りは型エラーとして検出できる。
//
// なお `char` を要素とする文字列 (`std::string`) は、
// 文字列エンコーディングが不明な文字列として扱う。
// (どのエンコーディングか状況から明らかなときや、
// char を要求する API を利用するときなどに使う。)
//
// ## 参考
//
// - [std::basic_string](https://cpprefjp.github.io/reference/string/basic_string.html)
// - [std::basic_string_view](https://cpprefjp.github.io/reference/string_view/basic_string_view.html)
// - [UTF-8文字列リテラル](https://cpprefjp.github.io/lang/cpp11/utf8_string_literals.html)
//
// その他: MSVC が C++20 に対応したら、Utf8Char は捨てて std::char8_t を使うとよい。
//
// - [UTF-8エンコーディングされた文字の型として`char8_t`を追加](https://cpprefjp.github.io/lang/cpp20/char8_t.html)

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

// Windows API が使うエンコーディングの文字列。
// UNICODE 版なら UTF-16。
// (ANSI 版 knowbug はもう提供されないので、常に UTF-16 と思ってよい。)
using OsString = std::basic_string<TCHAR>;

// Windows API が使うエンコーディングの文字列への参照。
using OsStringView = std::basic_string_view<TCHAR>;

// shift_jis (cp932) でエンコーディングされた文字列。
using SjisString = std::basic_string<SjisChar>;

// shift_jis 文字列への参照。
using SjisStringView = std::basic_string_view<SjisChar>;

// UTF-8 でエンコーディングされた文字列。
using Utf8String = std::basic_string<Utf8Char>;

// UTF-8 文字列への参照。
using Utf8StringView = std::basic_string_view<Utf8Char>;

// ASCII 文字列を UTF-8 エンコーディングとみなす。
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

// 文字列のコピーを作る。
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

// エンコーディング不明の文字列にキャストする。
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

// 文字列をストリームに出力する。
// (std::ostream が UTF-8 エンコーディングであると決めつけている。)
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
