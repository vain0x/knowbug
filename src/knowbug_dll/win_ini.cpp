#include "pch.h"
#include <array>
#include <vector>
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/platform.h"
#include "win_ini.h"

static auto const MIN_BUFFER_SIZE = std::size_t{ 1024 };
static auto const BOOL_BUFFER_SIZE = std::size_t{ 16 };

static auto string_to_lower(OsStringView value) -> OsString {
	auto it = to_owned(value);
	CharLower(it.data());
	return it;
}

static auto string_means_true(OsStringView value) -> bool {
	return value != TEXT("0") && string_to_lower(value) != TEXT("false");
}

static auto boolean_to_string(bool value) -> OsStringView {
	return value ? TEXT("true") : TEXT("false");
}

static auto ini_get_int(
	OsStringView section,
	OsStringView key,
	int default_value,
	OsStringView file_path
) -> int {
	return GetPrivateProfileInt(section.data(), key.data(), default_value, file_path.data());
}

static auto ini_get_string(
	OsStringView section,
	OsStringView key,
	OsStringView default_value,
	std::size_t buffer_size,
	OsStringView file_path
) -> OsString {
	auto buffer = std::vector<TCHAR>{};
	buffer.resize(buffer_size);

	auto len = GetPrivateProfileString(section.data(), key.data(), default_value.data(), buffer.data(), buffer.size(), file_path.data());

	return OsString{ buffer.data(), len };
}

auto IniFile::get_bool(char const* section, char const* key, bool default_value) const -> bool {
	auto value = ini_get_string(
		to_os(ascii_to_utf8(section)),
		to_os(ascii_to_utf8(key)),
		boolean_to_string(default_value),
		BOOL_BUFFER_SIZE,
		file_path_
	);
	return string_means_true(value);
}

auto IniFile::get_int(char const* section, char const* key, int default_value) const -> int {
	return ini_get_int(
		to_os(ascii_as_utf8(section)),
		to_os(ascii_as_utf8(key)),
		default_value,
		file_path_
	);
}

auto IniFile::get_string(
	char const* section,
	char const* key,
	char const* default_value,
	std::size_t buffer_size
) const -> OsString {
	return ini_get_string(
		to_os(ascii_to_utf8(section)),
		to_os(ascii_to_utf8(key)),
		to_os(ascii_to_utf8(default_value)),
		std::max(MIN_BUFFER_SIZE, buffer_size),
		file_path_
	);
}

auto IniFile::contains_key(char const* section, char const* key) const -> bool {
	// 既定値が返ってこないならキーがあるとみなす。
	return get_int(section, key, 0) != 0
		&& get_int(section, key, 1) != 1;
}
