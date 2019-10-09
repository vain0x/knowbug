// INIファイル読み書きクラス

#ifndef IG_CLASS_INI_H
#define IG_CLASS_INI_H

#include <vector>
#include <string>
#include "../knowbug_core/encoding.h"

class CIni {
	OsString const file_name_;
	std::vector<TCHAR> buf_;

public:
	explicit CIni(OsString&& file_name);

	bool getBool(char const* section, char const* key, bool defval_value = false);

	auto getInt(char const* section, char const* key, int default_value = 0) const -> int;

	auto getString(
		char const* section,
		char const* key,
		char const* default_value = "",
		size_t size = 0
	) -> OsStringView;

	bool existsKey(char const* sec, char const* key);

private:
	CIni(CIni const& obj) = delete;
};

#endif
