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

static auto ansi_to_os_str(const char* ansi_str, std::size_t ansi_str_len) -> OsString {
	assert(ansi_str != nullptr);
	assert(ansi_str_len == std::strlen(ansi_str));

	ansi_str_len++;

	auto len = MultiByteToWideChar(CP_ACP, 0, ansi_str, (int)ansi_str_len, nullptr, 0);
	assert(len >= 1);

	auto os_str = std::vector<TCHAR>(len, TCHAR{});
	MultiByteToWideChar(CP_ACP, 0, ansi_str, (int)ansi_str_len, os_str.data(), len);
	assert(os_str[len - 1] == TCHAR{});

	return OsString{ os_str.data() };
}

static auto utf8_to_os_str(char const* utf8_str, std::size_t utf8_str_len) -> OsString {
	assert(utf8_str != nullptr);
	assert(utf8_str_len == std::strlen(utf8_str));

	utf8_str_len++;

	auto len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, (int)utf8_str_len, nullptr, 0);
	assert(len >= 1);

	auto os_str = std::vector<TCHAR>(len, TCHAR{});
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, (int)utf8_str_len, os_str.data(), len);
	assert(os_str[len - 1] == TCHAR{});

	return OsString{ os_str.data() };
}

static auto os_to_ansi_str(LPCTSTR os_str, std::size_t os_str_len) -> std::string {
	assert(os_str != nullptr);
	assert(os_str_len == _tcslen(os_str));

	os_str_len++;

	auto len = WideCharToMultiByte(CP_ACP, 0, os_str, (int)os_str_len, nullptr, 0, nullptr, nullptr);
	assert(len >= 1);

	auto utf8_str = std::vector<char>(len, '\0');
	WideCharToMultiByte(CP_ACP, 0, os_str, (int)os_str_len, utf8_str.data(), len, nullptr, nullptr);
	assert(utf8_str[len - 1] == '\0');

	return std::string{ utf8_str.data() };
}

static auto os_to_utf8_str(LPCTSTR os_str) -> std::string {
	assert(os_str != nullptr);

	auto os_str_len = _tcslen(os_str) + 1;

	auto len = WideCharToMultiByte(CP_UTF8, 0, os_str, (int)os_str_len, nullptr, 0, nullptr, nullptr);
	assert(len >= 1);

	auto utf8_str = std::vector<char>(len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, os_str, (int)os_str_len, utf8_str.data(), len, nullptr, nullptr);
	assert(utf8_str[len - 1] == '\0');

	return std::string{ utf8_str.data() };
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
	return to_owned(ascii_as_utf8(source.data()));
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
	return OsString{ ansi_to_os_str((char const*)source.data(), source.size()) };
}

auto to_os(Utf8StringView const& source) -> OsString {
	return OsString{ utf8_to_os_str((char const*)source.data(), source.size()) };
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
	return Utf8String{ as_utf8(os_to_utf8_str(source.data())) };
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
