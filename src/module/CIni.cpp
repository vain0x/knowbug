// INIファイル読み書きクラス

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <array>

#include "CIni.h"

static auto const STR_BOOLEAN =
	std::array<char const*, 2> {{ "false", "true" }};

CIni::CIni(char const* fname)
	: fileName_(fname)
{
	buf_.resize(stc_defaultBufSize);
}

//------------------------------------------------
// [get] 論理値 (false | true; or 0 | non-0)
//------------------------------------------------
bool CIni::getBool(char const* sec, char const* key, bool defval) const
{
	GetPrivateProfileStringA
		( sec, key, STR_BOOLEAN[defval ? 1 : 0]
		, buf(), buf_.size(), fileName_.c_str()
		);
	CharLower(buf());
	return !(strcmp(buf(), "0") == 0 || strcmp(buf(), STR_BOOLEAN[0]) == 0);
}

//------------------------------------------------
// [get] 整数値
//------------------------------------------------
int CIni::getInt(char const* sec, char const* key, int defval) const
{
	return GetPrivateProfileIntA(sec, key, defval, fileName_.c_str());
}

//------------------------------------------------
// [get] 文字列
//------------------------------------------------
char const* CIni::getString(char const* sec, char const* key, char const* defval, size_t size) const
{
	if ( size > buf_.size() ) buf_.resize(size);

	GetPrivateProfileStringA(sec, key, defval, buf(), buf_.size(), fileName_.c_str());
	return buf();
}

//------------------------------------------------
// [set] 論理値
//------------------------------------------------
void CIni::setBool(char const* sec, char const* key, bool val)
{
	strcpy_s(buf(), buf_.size(), STR_BOOLEAN[val ? 1 : 0]);
	WritePrivateProfileStringA(sec, key, buf(), fileName_.c_str());
}

//------------------------------------------------
// [set] 整数値
//------------------------------------------------
void CIni::setInt(char const* sec, char const* key, int val, int radix)
{
	_itoa_s(val, buf(), buf_.size(), radix);

	WritePrivateProfileStringA(sec, key, buf(), fileName_.c_str());
}

//------------------------------------------------
// [set] 文字列
//------------------------------------------------
void CIni::setString(char const* sec, char const* key, char const* val)
{
	sprintf_s(buf(), buf_.size(), "\"%s\"", val);
	WritePrivateProfileStringA(sec, key, buf(), fileName_.c_str());
}

//------------------------------------------------
// セクションの列挙
//------------------------------------------------
std::vector<std::string> CIni::enumSections() const
{
	return enumImpl(nullptr);
}

//------------------------------------------------
// キーの列挙
//------------------------------------------------
std::vector<std::string> CIni::enumKeys(char const* sec) const
{
	return enumImpl(sec);
}

//------------------------------------------------
// 列挙
//------------------------------------------------
static std::vector<std::string> splitByNullChar(char const* buf, size_t size);

std::vector<std::string> CIni::enumImpl(char const* secOrNull) const
{
	auto const size =
		GetPrivateProfileString
			( secOrNull, nullptr, nullptr
			, buf(), buf_.size(), fileName_.c_str()
			);

	// バッファ不足
	if ( size == buf_.size() - 2 ) {
		buf_.resize(buf_.size() * 2 + 1);
		return enumImpl(secOrNull);
	}
	return splitByNullChar(buf(), size);
}

// '\0' 区切り文字列、終端は2連続の '\0'
std::vector<std::string> splitByNullChar(char const* buf, size_t size)
{
	auto ls = std::vector<std::string> {};
	if ( size != 0 ) {
		auto idx = size_t { 0 };
		for (;;) {
			assert(idx < size);
			std::string const s = &buf[idx];
			idx += s.length() + 1;
			ls.emplace_back(std::move(s));
			if ( buf[idx] == '\0' ) break;
		}
	}
	return std::move(ls);
}

//------------------------------------------------
// セクションを削除する
//------------------------------------------------
void CIni::removeSection(char const* sec)
{
	WritePrivateProfileStringA(sec, nullptr, nullptr, fileName_.c_str());
}

//------------------------------------------------
// キーを削除する
//------------------------------------------------
void CIni::removeKey(char const* sec, char const* key)
{
	WritePrivateProfileStringA(sec, key, nullptr, fileName_.c_str());
}

//------------------------------------------------
// キーの有無
//------------------------------------------------
bool CIni::existsKey(char const* sec, char const* key) const
{
	return (getInt(sec, key, 0) != 0)
		&& (getInt(sec, key, 1) != 1)
		;
}
