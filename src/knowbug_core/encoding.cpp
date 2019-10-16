#include "pch.h"
#include <cassert>
#include <memory>
#include <string>
#include <tchar.h>
#include <vector>
#include "encoding.h"

#ifdef HSP3_UTF8

static auto cast_from_hsp(HspStringView const& source) -> Utf8StringView const& {
	return (Utf8StringView const&)source;
}

static auto cast_from_hsp(HspString&& source) -> Utf8String&& {
	return (Utf8String&&)source;
}

static auto cast_to_hsp(Utf8StringView const& source) -> HspStringView const& {
	return (HspStringView const&)source;
}

static auto cast_to_hsp(Utf8String&& source) -> HspString&& {
	return (HspString&&)source;
}

#else

static auto cast_from_hsp(HspStringView const& source) -> SjisStringView const& {
	return (SjisStringView const&)source;
}

static auto cast_from_hsp(HspString&& source) -> SjisString&& {
	return (SjisString&&)source;
}

static auto cast_to_hsp(SjisStringView const& source) -> HspStringView const& {
	return (HspStringView const&)source;
}

static auto cast_to_hsp(SjisString&& source) -> HspString&& {
	return (HspString&&)source;
}

#endif

static auto string_is_ascii(char const* str) -> bool {
	if (str == nullptr) {
		return false;
	}

	auto p = (unsigned char const*)str;
	while (*p) {
		if (*p >= 0x7f) {
			return false;
		}
		p++;
	}
	return true;
}

static auto fail_convert_to_os_str() -> OsString {
	// FIXME: エラーログ？
	assert(false && u8"can't convert to unicode");
	return OsString{ TEXT("<文字列を解釈できません>") };
}

static auto fail_convert_to_ansi_str() -> std::string {
	// FIXME: エラーログ？
	assert(false && u8"can't convert to ansi");
	return std::string{ u8"<invalid string>" };
}

static auto fail_convert_to_utf8_str() -> Utf8String {
	// FIXME: エラーログ？
	assert(false && u8"can't convert to utf-8");
	return Utf8String{ as_utf8(u8"<文字列を解釈できません>") };
}

// 参考: https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
static auto ansi_to_os_str(char const* ansi_str, std::size_t ansi_str_len) -> OsString {
	assert(ansi_str != nullptr);
	assert(ansi_str_len <= std::strlen(ansi_str));

	if (ansi_str_len == 0) {
		return OsString{};
	}

	// 変換後の文字列のサイズを計算する。いま ansi_str_len > 0 なので、バッファサイズに NULL 文字分は含まれない。
	auto len = MultiByteToWideChar(CP_ACP, 0, ansi_str, (int)ansi_str_len, LPTSTR{}, 0);
	if (len == 0) {
		return fail_convert_to_os_str();
	}
	assert(len > 0);

	auto os_str = OsString{};
	os_str.resize(len);

	// 文字コードの変換を行う。
	auto result = MultiByteToWideChar(CP_ACP, 0, ansi_str, (int)ansi_str_len, os_str.data(), len);
	if (result == 0) {
		return fail_convert_to_os_str();
	}
	assert(result == len);

	return os_str;
}

static auto utf8_to_os_str(Utf8Char const* utf8_str, std::size_t utf8_str_len) -> OsString {
	assert(utf8_str != nullptr);
	assert(utf8_str_len <= std::strlen((char const*)utf8_str));

	if (utf8_str_len == 0) {
		return OsString{};
	}

	auto len = MultiByteToWideChar(CP_UTF8, 0, (char const*)utf8_str, (int)utf8_str_len, LPTSTR{}, 0);
	if (len == 0) {
		return fail_convert_to_os_str();
	}
	assert(len > 0);

	auto os_str = OsString{};
	os_str.resize(len);

	auto result = MultiByteToWideChar(CP_UTF8, 0, (char const*)utf8_str, (int)utf8_str_len, os_str.data(), len);
	if (result == 0) {
		return fail_convert_to_os_str();
	}
	assert(result == len);

	return os_str;
}

// 参考: https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
static auto os_to_ansi_str(LPCTSTR os_str, std::size_t os_str_len) -> std::string {
	assert(os_str != nullptr);
	assert(os_str_len <= _tcslen(os_str));

	if (os_str_len == 0) {
		return std::string{};
	}

	auto len = WideCharToMultiByte(CP_ACP, 0, os_str, (int)os_str_len, nullptr, 0, nullptr, nullptr);
	if (len == 0) {
		return fail_convert_to_ansi_str();
	}
	assert(len > 0);

	auto ansi_str = std::string{};
	ansi_str.resize(len);

	auto result = WideCharToMultiByte(CP_ACP, 0, os_str, (int)os_str_len, ansi_str.data(), len, nullptr, nullptr);
	if (result == 0) {
		return fail_convert_to_ansi_str();
	}
	assert(result == len);

	return ansi_str;
}

static auto os_to_utf8_str(LPCTSTR os_str, std::size_t os_str_len) -> Utf8String {
	assert(os_str != nullptr);
	assert(os_str_len <= _tcslen(os_str));

	if (os_str_len == 0) {
		return Utf8String{};
	}

	auto len = WideCharToMultiByte(CP_UTF8, 0, os_str, (int)os_str_len, nullptr, 0, nullptr, nullptr);
	if (len == 0) {
		return fail_convert_to_utf8_str();
	}
	assert(len > 0);

	auto utf8_str = Utf8String{};
	utf8_str.resize(len);

	auto result = WideCharToMultiByte(CP_UTF8, 0, os_str, (int)os_str_len, (char*)utf8_str.data(), len, nullptr, nullptr);
	if (result == 0) {
		return fail_convert_to_utf8_str();
	}
	assert(result == len);

	return utf8_str;
}

auto ascii_as_utf8(char const* source) -> Utf8StringView {
	assert(string_is_ascii(source));
	return Utf8StringView{ (Utf8Char const*)source };
}

auto ascii_as_utf8(std::string&& source) -> Utf8String {
	assert(string_is_ascii(source.data()));
	return Utf8String{ (Utf8String&&)source };
}

auto ascii_to_utf8(std::string const& source) -> Utf8String {
	assert(string_is_ascii(source.data()));
	return Utf8String{ (Utf8String const&)source };
}

auto as_hsp(char const* str) -> HspStringView {
#ifdef HSP3_UTF8
	return cast_to_hsp(as_utf8(str));
#else
	return cast_to_hsp(as_sjis(str));
#endif
}

auto as_hsp(std::string_view const& str) -> HspStringView {
#ifdef HSP3_UTF8
	return cast_to_hsp(as_utf8(str));
#else
	return cast_to_hsp(as_sjis(str));
#endif
}

auto as_hsp(std::string&& str) -> HspString {
	return HspString{ (HspString&&)str };
}

auto to_hsp(OsStringView const& source) -> HspString {
#ifdef HSP3_UTF8
	return cast_to_hsp(to_utf8(source));
#else
	return cast_to_hsp(to_sjis(source));
#endif
}

auto to_hsp(SjisStringView const& source) -> HspString {
#ifdef HSP3_UTF8
	return cast_to_hsp(to_utf8(source));
#else
	return cast_to_hsp(to_owned(source));
#endif
}

auto to_hsp(Utf8StringView const& source) -> HspString {
#ifdef HSP3_UTF8
	return cast_to_hsp(to_owned(source));
#else
	return cast_to_hsp(to_sjis(source));
#endif
}

auto as_sjis(char const* str) -> SjisStringView {
	return SjisStringView{ (SjisChar const*)str };
}

auto as_sjis(std::string_view const& str) -> SjisStringView {
	return SjisStringView{ (SjisChar const*)str.data(), str.size() };
}

auto as_sjis(std::string&& source) -> SjisString {
	return SjisString{ (SjisString&&)source };
}

auto to_sjis(HspStringView const& source) -> SjisString {
#ifdef HSP3_UTF8
	return to_sjis(cast_from_hsp(source));
#else
	return to_owned(cast_from_hsp(source));
#endif
}

auto to_sjis(OsStringView const& source) -> SjisString {
	return SjisString{ as_sjis(os_to_ansi_str(source.data(), source.size())) };
}

auto to_sjis(Utf8StringView const& source) -> SjisString {
	return to_sjis(to_os(source));
}

auto as_os(LPCTSTR str) -> OsStringView {
	return OsStringView{ str, std::size_t{} };
}

auto to_os(HspStringView const& source) -> OsString {
	return to_os(cast_from_hsp(source));
}

auto to_os(SjisStringView const& source) -> OsString {
	return ansi_to_os_str((char const*)source.data(), source.size());
}

auto to_os(Utf8StringView const& source) -> OsString {
	return utf8_to_os_str(source.data(), source.size());
}

auto as_utf8(char const* str) -> Utf8StringView {
	return Utf8StringView{ (Utf8Char const*)str };
}

auto as_utf8(std::string_view const& str) -> Utf8StringView {
	return Utf8StringView{ (Utf8Char const*)str.data(), str.size() };
}

auto as_utf8(std::string&& source) -> Utf8String {
	return Utf8String{ (Utf8String&&)source };
}

auto to_utf8(HspStringView const& source) -> Utf8String {
#ifdef HSP3_UTF8
	return to_owned(cast_from_hsp(source));
#else
	return to_utf8(cast_from_hsp(source));
#endif
}

auto to_utf8(OsStringView const& source) -> Utf8String {
	return os_to_utf8_str(source.data(), source.size());
}

auto to_utf8(SjisStringView const& source) -> Utf8String {
	return to_utf8(to_os(source));
}

auto to_owned(HspStringView const& source) -> HspString {
	return HspString{ source.begin(), source.end() };
}

auto to_owned(OsStringView const& source) -> OsString {
	return OsString{ source.begin(), source.end() };
}

auto to_owned(SjisStringView const& source) -> SjisString {
	return SjisString{ source.begin(), source.end() };
}

auto to_owned(Utf8StringView const& source) -> Utf8String {
	return Utf8String{ source.begin(), source.end() };
}

auto as_view(HspString const& source) -> HspStringView {
	return HspStringView{ source };
}

auto as_view(OsString const& source) -> OsStringView {
	return OsStringView{ source };
}

auto as_view(SjisString const& source) -> SjisStringView {
	return SjisStringView{ source };
}

auto as_view(Utf8String const& source) -> Utf8StringView {
	return Utf8StringView{ source };
}

auto as_native(HspStringView const& source) -> std::string_view {
	return std::string_view{ (std::string_view const&)source };
}

auto as_native(SjisStringView const& source) -> std::string_view {
	return std::string_view{ (std::string_view const&)source };
}

auto as_native(Utf8StringView const& source) -> std::string_view {
	return std::string_view{ (std::string_view const&)source };
}

auto as_native(HspString&& source) -> std::string {
	return std::string{ (std::string&&)source };
}

auto as_native(SjisString&& source) -> std::string {
	return std::string{ (std::string&&)source };
}

auto as_native(Utf8String&& source) -> std::string {
	return std::string{ (std::string&&)source };
}
