// INI ファイルの読み込み機能を提供する。

#pragma once

#include <string>
#include <vector>
#include "../knowbug_core/encoding.h"

class IniFile {
	OsString file_path_;

public:
	explicit IniFile(OsString&& file_path)
		: file_path_(std::move(file_path))
	{
	}

	// 指定したキーに対応する値を論理値として取得する。
	// 0 と false は偽。
	auto get_bool(char const* section, char const* key, bool defval_value = false) const -> bool;

	auto get_int(char const* section, char const* key, int default_value = 0) const -> int;

	auto get_string(
		char const* section,
		char const* key,
		char const* default_value = "",
		std::size_t buffer_size = 0
	) const -> OsString;

	auto contains_key(char const* section, char const* key) const -> bool;

private:
	IniFile(IniFile const& other) = delete;
};
