// INIファイル読み書きクラス

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <array>
#include <tchar.h>
#include "../encoding.h"
#include "CIni.h"

static auto const DEFAULT_BUFFER_SIZE = std::size_t{ 1024 };

static auto const STR_BOOLEAN = std::array<TCHAR const*, 2> {{ TEXT("false"), TEXT("true") }};

CIni::CIni(OsString&& file_name)
	: file_name_(std::move(file_name))
{
	buf_.resize(1024);
}

//------------------------------------------------
// [get] 論理値 (false | true; or 0 | non-0)
//------------------------------------------------
bool CIni::getBool(char const* sec, char const* key, bool defval)
{
	auto sec_str = to_os(ascii_to_utf8(sec));
	auto key_str = to_os(ascii_to_utf8(key));

	GetPrivateProfileString(
		sec_str.data(), key_str.data(), STR_BOOLEAN[defval ? 1 : 0],
		(LPTSTR)buf_.data(), buf_.size(), file_name_.data()
	);
	CharLower((LPTSTR)buf_.data());

	auto text = OsStringView{ buf_.data() };
	auto is_false = text == OsStringView{ TEXT("0") } || text == OsStringView{ TEXT("false") };

	return !is_false;
}

//------------------------------------------------
// [get] 整数値
//------------------------------------------------
auto CIni::getInt(char const* sec, char const* key, int default_value) const -> int
{
	auto sec_str = to_os(ascii_to_utf8(sec));
	auto key_str = to_os(ascii_to_utf8(key));

	return GetPrivateProfileInt(
		sec_str.data(), key_str.data(), default_value,
		file_name_.data()
	);
}

//------------------------------------------------
// [get] 文字列
//------------------------------------------------
auto CIni::getString(char const* sec, char const* key, char const* defval, size_t size)
-> OsStringView
{
	auto sec_str = to_os(ascii_to_utf8(sec));
	auto key_str = to_os(ascii_to_utf8(key));
	auto default_str = to_os(ascii_to_utf8(defval));

	if ( size > buf_.size() ) buf_.resize(size);

	GetPrivateProfileString(
		sec_str.data(), key_str.data(), default_str.data(),
		(LPTSTR)buf_.data(), buf_.size(), file_name_.data()
	);
	return OsStringView{ buf_.data() };
}

//------------------------------------------------
// キーの有無
//------------------------------------------------
bool CIni::existsKey(char const* sec, char const* key)
{
	return (getInt(sec, key, 0) != 0) && (getInt(sec, key, 1) != 1);
}
