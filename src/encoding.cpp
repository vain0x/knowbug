#include <cassert>
#include <memory>
#include <string>
#include <tchar.h>
#include <vector>
#include "encoding.h"

using BasicOsString = std::basic_string<TCHAR>;

auto string_is_ascii(char const* str) -> bool {
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

static auto ansi_to_os_str(const char* ansi_str) -> BasicOsString {
	assert(ansi_str != nullptr);

	auto ansi_str_len = std::strlen(ansi_str) + 1;

	auto len = MultiByteToWideChar(CP_ACP, 0, ansi_str, (int)ansi_str_len, nullptr, 0);
	assert(len >= 1);

	auto os_str = std::vector<TCHAR>(len, TCHAR{});
	MultiByteToWideChar(CP_ACP, 0, ansi_str, (int)ansi_str_len, os_str.data(), len);
	assert(os_str[len - 1] == TCHAR{});

	return BasicOsString{ os_str.data() };
}

static auto utf8_to_os_str(char const* utf8_str) -> BasicOsString {
	assert(utf8_str != nullptr);

	auto utf8_str_len = std::strlen(utf8_str) + 1;

	auto len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, (int)utf8_str_len, nullptr, 0);
	assert(len >= 1);

	auto os_str = std::vector<TCHAR>(len, TCHAR{});
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, (int)utf8_str_len, os_str.data(), len);
	assert(os_str[len - 1] == TCHAR{});

	return BasicOsString{ os_str.data() };
}

static auto os_to_ansi_str(LPCTSTR os_str) -> std::string {
	assert(os_str != nullptr);

	auto os_str_len = _tcslen(os_str) + 1;

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

auto OsStringView::to_hsp_string() const -> HspString {
#ifdef HSP3_UTF8
	return to_utf8_string();
#else
	return to_sjis_string();
#endif
}

auto OsStringView::to_owned() const -> OsString {
	return OsString{ BasicOsString{ data() } };
}

auto OsStringView::to_sjis_string() const -> SjisString {
	return SjisString{ os_to_ansi_str(data()) };
}

auto OsStringView::to_utf8_string() const -> Utf8String {
	return Utf8String{ os_to_utf8_str(data()) };
}

auto OsString::to_hsp_string() const -> HspString {
	return as_ref().to_hsp_string();
}

auto OsString::to_sjis_string() const -> SjisString {
	return as_ref().to_sjis_string();
}

auto OsString::to_utf8_string() const -> Utf8String {
	return as_ref().to_utf8_string();
}

auto SjisStringView::to_owned() const -> SjisString {
	return SjisString{ std::string{data()} };
}

auto SjisStringView::to_hsp_string() const -> HspString {
#ifdef HSP3_UTF8
	return to_utf8_string();
#else
	return to_owned();
#endif
}

auto SjisStringView::to_os_string() const -> OsString {
	return OsString{ ansi_to_os_str(data()) };
}

auto SjisStringView::to_utf8_string() const -> Utf8String {
	return to_os_string().to_utf8_string();
}

auto SjisString::to_hsp_string() const -> HspString {
	return as_ref().to_hsp_string();
}

auto SjisString::to_os_string() const -> OsString {
	return as_ref().to_os_string();
}

auto SjisString::to_utf8_string() const -> Utf8String {
	return as_ref().to_utf8_string();
}

auto Utf8StringView::to_owned() const -> Utf8String {
	return Utf8String{ std::string{ data() } };
}

auto Utf8StringView::to_hsp_string() const -> HspString {
#ifdef HSP3_UTF8
	return to_owned();
#else
	return to_sjis_string();
#endif
}

auto Utf8StringView::to_os_string() const -> OsString {
	return OsString{ utf8_to_os_str(data()) };
}

auto Utf8StringView::to_sjis_string() const -> SjisString {
	return to_os_string().to_sjis_string();
}
