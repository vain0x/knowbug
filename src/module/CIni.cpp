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

/// Win32 API で使う文字列を HSP が使用する文字コードに変換する。
HSPCHAR *api_to_hsp_str(const HSPAPICHAR *api_str, size_t* hsp_str_len)
{
	if (api_str == nullptr || hsp_str_len == nullptr) throw std::invalid_argument{ "never null" };

	// api_str をゼロ終端とみなして、変換後のバッファサイズを計算する。
	auto len = WideCharToMultiByte(CP_UTF8, 0, api_str, -1, nullptr, 0, nullptr, nullptr);
	assert(len >= 1);

	auto hsp_str = (HSPCHAR *)calloc(len, sizeof(HSPCHAR));
	WideCharToMultiByte(CP_UTF8, 0, api_str, -1, hsp_str, len, nullptr, nullptr);

	*hsp_str_len = len;
	return hsp_str;
}

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
	if ( size > buf_.size() ) buf_.resize(size);

	GetPrivateProfileString(chartoapichar(sec,&hactmp1), chartoapichar(key,&hactmp2), chartoapichar(defval,&hactmp3), buf(), buf_.size(), chartoapichar(fileName_.c_str(),&hactmp4));
	freehac(&hactmp1);
	freehac(&hactmp2);
	freehac(&hactmp3);
	freehac(&hactmp4);

	// 文字コードの変換を行う。
	{
		size_t len;
		auto value = api_to_hsp_str(buf(), &len);
		if (len > buf8_.size()) buf8_.resize(len);
		std::memcpy(buf8_.data(), value, len);
		freehc(&value);
	}

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
// キーの有無
//------------------------------------------------
bool CIni::existsKey(char const* sec, char const* key) const
{
	return (getInt(sec, key, 0) != 0)
		&& (getInt(sec, key, 1) != 1)
		;
}
