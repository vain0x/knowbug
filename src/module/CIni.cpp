// INIファイル読み書きクラス

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <array>

#include <tchar.h>

#include "CIni.h"

static auto const STR_BOOLEAN =
	std::array<HSPAPICHAR const*, 2> {{ TEXT("false"), TEXT("true") }};

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
	HSPAPICHAR *hactmp1;
	HSPAPICHAR *hactmp2;
	HSPAPICHAR *hactmp3;
	GetPrivateProfileString
		( chartoapichar(sec,&hactmp1), chartoapichar(key,&hactmp2), STR_BOOLEAN[defval ? 1 : 0]
		, buf(), buf_.size(), chartoapichar(fileName_.c_str(),&hactmp3)
		);
	freehac(&hactmp1);
	freehac(&hactmp2);
	freehac(&hactmp3);
	CharLower(buf());
	return !(_tcscmp(buf(), TEXT("0")) == 0 || _tcscmp(buf(), STR_BOOLEAN[0]) == 0);
}

//------------------------------------------------
// [get] 整数値
//------------------------------------------------
auto CIni::getInt(char const* sec, char const* key, int defval) const -> int
{
	HSPAPICHAR *hactmp1;
	HSPAPICHAR *hactmp2;
	HSPAPICHAR *hactmp3;
	return GetPrivateProfileInt(chartoapichar(sec,&hactmp1), chartoapichar(key,&hactmp2), defval, chartoapichar(fileName_.c_str(),&hactmp3));
	freehac(&hactmp1);
	freehac(&hactmp2);
	freehac(&hactmp3);
}

//------------------------------------------------
// [get] 文字列
//------------------------------------------------
auto CIni::getString(char const* sec, char const* key, char const* defval, size_t size) const
-> char const*
{
	HSPAPICHAR *hactmp1;
	HSPAPICHAR *hactmp2;
	HSPAPICHAR *hactmp3;
	HSPAPICHAR *hactmp4;
	HSPCHAR *hctmp1;
	size_t len;
	if ( size > buf_.size() ) buf_.resize(size);

	GetPrivateProfileString(chartoapichar(sec,&hactmp1), chartoapichar(key,&hactmp2), chartoapichar(defval,&hactmp3), buf(), buf_.size(), chartoapichar(fileName_.c_str(),&hactmp4));
	freehac(&hactmp1);
	freehac(&hactmp2);
	freehac(&hactmp3);
	freehac(&hactmp4);
	apichartohspchar(buf(), &hctmp1);
	len = strlen(hctmp1);
	if (len >= buf8_.size()) buf8_.resize(size+1);
	memcpy(buf8_.data(), hctmp1, len);
	buf8_.data()[len] = 0;
	freehc(&hctmp1);
	return buf8();
}

//------------------------------------------------
// [set] 論理値
//------------------------------------------------
void CIni::setBool(char const* sec, char const* key, bool val)
{
	HSPAPICHAR *hactmp1;
	HSPAPICHAR *hactmp2;
	HSPAPICHAR *hactmp3;
	_tcscpy_s(buf(), buf_.size(), STR_BOOLEAN[val ? 1 : 0]);
	WritePrivateProfileString(chartoapichar(sec,&hactmp1), chartoapichar(key,&hactmp2), buf(), chartoapichar(fileName_.c_str(),&hactmp3));
	freehac(&hactmp1);
	freehac(&hactmp2);
	freehac(&hactmp3);
}

//------------------------------------------------
// [set] 整数値
//------------------------------------------------
void CIni::setInt(char const* sec, char const* key, int val, int radix)
{
	HSPAPICHAR *hactmp1;
	HSPAPICHAR *hactmp2;
	HSPAPICHAR *hactmp3;
	_itot_s(val, buf(), buf_.size(), radix);

	WritePrivateProfileString(chartoapichar(sec,&hactmp1), chartoapichar(key,&hactmp2), buf(), chartoapichar(fileName_.c_str(),&hactmp3));
	freehac(&hactmp1);
	freehac(&hactmp2);
	freehac(&hactmp3);
}

//------------------------------------------------
// [set] 文字列
//------------------------------------------------
void CIni::setString(char const* sec, char const* key, char const* val)
{
	HSPAPICHAR *hactmp1;
	HSPAPICHAR *hactmp2;
	HSPAPICHAR *hactmp3;
	_tprintf_s(buf(), buf_.size(), "\"%s\"", val);
	WritePrivateProfileString(chartoapichar(sec,&hactmp1), chartoapichar(key,&hactmp2), buf(), chartoapichar(fileName_.c_str(),&hactmp3));
	freehac(&hactmp1);
	freehac(&hactmp2);
	freehac(&hactmp3);
}

//------------------------------------------------
// セクションの列挙
//------------------------------------------------
auto CIni::enumSections() const -> std::vector<std::string>
{
	return enumImpl(nullptr);
}

//------------------------------------------------
// キーの列挙
//------------------------------------------------
auto CIni::enumKeys(char const* sec) const -> std::vector<std::string>
{
	return enumImpl(sec);
}

//------------------------------------------------
// 列挙
//------------------------------------------------
static auto splitByNullChar(char const* buf, size_t size) -> std::vector<std::string>;

auto CIni::enumImpl(char const* secOrNull) const -> std::vector<std::string>
{
	HSPAPICHAR *hactmp1;
	HSPAPICHAR *hactmp2;
	HSPCHAR *hctmp1;
	size_t len;
	auto const size =
		GetPrivateProfileString
			( chartoapichar(secOrNull,&hactmp1), nullptr, nullptr
			, buf(), buf_.size(), chartoapichar(fileName_.c_str(),&hactmp2)
			);
	freehac(&hactmp1);
	freehac(&hactmp2);

	// バッファ不足
	if ( size == buf_.size() - 2 ) {
		buf_.resize(buf_.size() * 2 + 1);
		return enumImpl(secOrNull);
	}
	apichartohspchar(buf(), &hctmp1);
	len = strlen(hctmp1);
	if (len >= buf8_.size()) buf8_.resize(size + 1);
	memcpy(buf8_.data(), hctmp1, len);
	buf8_.data()[len] = 0;
	freehc(&hctmp1);
	return splitByNullChar(buf8(), size);
}

// '\0' 区切り文字列、終端は2連続の '\0'
auto splitByNullChar(char const* buf, size_t size) -> std::vector<std::string>
{
	auto ls = std::vector<std::string> {};
	if ( size != 0 ) {
		auto idx = size_t { 0 };
		for (;;) {
			assert(idx < size);
			auto s = std::string { &buf[idx] };
			idx += s.length() + 1;
			ls.emplace_back(std::move(s));
			if ( buf[idx] == '\0' ) break;
		}
	}
	return ls;
}

//------------------------------------------------
// セクションを削除する
//------------------------------------------------
void CIni::removeSection(char const* sec)
{
	HSPAPICHAR *hactmp1;
	HSPAPICHAR *hactmp2;
	WritePrivateProfileString(chartoapichar(sec,&hactmp1), nullptr, nullptr, chartoapichar(fileName_.c_str(),&hactmp2));
	freehac(&hactmp1);
	freehac(&hactmp2);
}

//------------------------------------------------
// キーを削除する
//------------------------------------------------
void CIni::removeKey(char const* sec, char const* key)
{
	HSPAPICHAR *hactmp1;
	HSPAPICHAR *hactmp2;
	HSPAPICHAR *hactmp3;
	WritePrivateProfileString(chartoapichar(sec,&hactmp1), chartoapichar(key,&hactmp2), nullptr, chartoapichar(fileName_.c_str(),&hactmp3));
	freehac(&hactmp1);
	freehac(&hactmp2);
	freehac(&hactmp3);
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
