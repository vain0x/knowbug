// INIファイル読み書きクラス

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include "CIni.h"

static char const* const STR_BOOLEAN[2] = { "false", "true" };

//------------------------------------------------
// 構築
//------------------------------------------------
CIni::CIni( char const* fname )
	: msFilename( nullptr )
	, msBuf     ( nullptr )
	, msizeBuf  ( stc_defaultBufSize )
{
	msFilename = reinterpret_cast<char*>( std::malloc( (MAX_PATH + 1) * sizeof(char) ) );
	msBuf      = reinterpret_cast<char*>( std::malloc( msizeBuf * sizeof(char) ) );
	
	if ( fname ) open( fname );
	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CIni::~CIni()
{
	close();
	
	std::free( msFilename ); msFilename = nullptr;
	std::free( msBuf ); msBuf = nullptr;
	return;
}

//------------------------------------------------
// ファイルを開く
//------------------------------------------------
void CIni::open( char const* fname )
{
	if ( !fname ) return;
	close();
	
	strcpy_s( msFilename, MAX_PATH, fname );
	return;
}

//------------------------------------------------
// ファイルを閉じる
//------------------------------------------------
void CIni::close()
{
	msFilename[0] = '\0';
	return;
}

//------------------------------------------------
// [get] 論理値 (false | true, or 0 | non-0)
//------------------------------------------------
bool CIni::getBool( char const* sec, char const* key, bool defval )
{
	const size_t MinBufForBooleanString = 8;
	reserve( MinBufForBooleanString );
	GetPrivateProfileStringA( sec, key, STR_BOOLEAN[defval ? 1 : 0], msBuf, MinBufForBooleanString - 1, msFilename );

	CharLower( msBuf );		// 小文字にする
	
	return !( strcmp(msBuf, "0") == 0 || strcmp(msBuf, STR_BOOLEAN[0]) == 0 );
}

//------------------------------------------------
// [get] 整数値
//------------------------------------------------
int CIni::getInt( char const* sec, char const* key, int defval )
{
	return GetPrivateProfileIntA( sec, key, defval, msFilename );
}

//------------------------------------------------
// [get] 文字列
//------------------------------------------------
char const* CIni::getString( char const* sec, char const* key, char const* defval, size_t size )
{
	if ( size ) reserve( size );
	
	GetPrivateProfileStringA( sec, key, defval, msBuf, msizeBuf - 1, msFilename );
	return msBuf;
}

//------------------------------------------------
// [set] 論理値
//------------------------------------------------
void CIni::setBool( char const* sec, char const* key, bool val )
{
	strcpy_s( msBuf, msizeBuf, STR_BOOLEAN[val ? 1 : 0] );		// 文字列で書き込む
	
	WritePrivateProfileStringA( sec, key, msBuf, msFilename );
	return;
}

//------------------------------------------------
// [set] 整数値
//------------------------------------------------
void CIni::setInt( char const* sec, char const* key, int val, int radix )
{
	_itoa_s( val, msBuf, msizeBuf, radix );
	
	WritePrivateProfileStringA( sec, key, msBuf, msFilename );
	return;
}

//------------------------------------------------
// [set] 文字列
//------------------------------------------------
void CIni::setString( char const* sec, char const* key, char const* val )
{
	sprintf_s( msBuf, msizeBuf, "\"%s\"", val );
	
	WritePrivateProfileStringA( sec, key, msBuf, msFilename );
	return;
}

//------------------------------------------------
// セクションの列挙
//------------------------------------------------
std::list<std::string> CIni::enumSections()
{
	return enumImpl(nullptr);
}

//------------------------------------------------
// キーの列挙
//------------------------------------------------
std::list<std::string> CIni::enumKeys(char const* sec)
{
	return enumImpl(sec);
}

//------------------------------------------------
// 列挙
//------------------------------------------------
static std::list<std::string> splitByNullChar(char const* buf, size_t size);

std::list<std::string> CIni::enumImpl(char const* secOrNull)
{
	size_t const size =
		GetPrivateProfileString(secOrNull, nullptr, nullptr, msBuf, msizeBuf, msFilename);
	
	// バッファ不足
	if ( size == msizeBuf - 2 ) {
		expand(msizeBuf);
		return enumImpl(secOrNull);
	}

	return splitByNullChar(msBuf, size);
}

// '\0' 区切り文字列、終端は2連続の '\0'
std::list<std::string> splitByNullChar(char const* buf, size_t size)
{
	std::list<std::string> ls;

	if ( size != 0 ) {
		size_t idx = 0;
		for (;;) {
			assert(idx < size);
			std::string const s { &buf[idx] };
			idx += s.length() + 1;
			ls.push_back(std::move(s));
			if ( buf[idx] == '\0' ) break;
		}
	}
	return std::move(ls);
}

//------------------------------------------------
// セクションを削除する
//------------------------------------------------
void CIni::removeSection( char const* sec )
{
	WritePrivateProfileStringA( sec, nullptr, nullptr, msFilename );
	return;
}

//------------------------------------------------
// キーを削除する
//------------------------------------------------
void CIni::removeKey( char const* sec, char const* key )
{
	WritePrivateProfileStringA( sec, key, nullptr, msFilename );
	return;
}

//------------------------------------------------
// キーの有無
//------------------------------------------------
bool CIni::existsKey( char const* sec, char const* key )
{
	return ( getInt( sec, key, 0 ) != 0 )
		&& ( getInt( sec, key, 1 ) != 1 )
	;
}

//------------------------------------------------
// バッファを拡張する
//------------------------------------------------
void CIni::reserve( size_t newSize )
{
	if ( newSize < msizeBuf ) return;
	
	msizeBuf = newSize + 1;
	msBuf    = reinterpret_cast<char*>( std::realloc( msBuf, msizeBuf * sizeof(char) ) );
	return;
}
