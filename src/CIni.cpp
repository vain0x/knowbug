// INIファイル読み書きクラス

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "CIni.h"

//##############################################################################
//                定義部 : CIni
//##############################################################################
//------------------------------------------------
// 構築
//------------------------------------------------
CIni::CIni( const char *fname )
	: msFilename( NULL )
	, msBuf     ( NULL )
	, msizeBuf  ( stc_defaultBufSize )
{
	msFilename = (char *)std::malloc( (MAX_PATH + 1) * sizeof(char) );
	msBuf      = (char *)std::malloc( msizeBuf * sizeof(char) );
	
	if ( fname ) open( fname );
	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CIni::~CIni()
{
	close();
	
	if ( msFilename != NULL ) std::free( msFilename );
	if ( msBuf      != NULL ) std::free( msBuf );
	
	msFilename = NULL;
	msBuf      = NULL;
	msizeBuf   = 0;
	return;
}

//------------------------------------------------
// ファイルを開く
//------------------------------------------------
void CIni::open( const char *fname )
{
	if ( !fname ) return;
	
	close();
	
	strcpy_s( msFilename, MAX_PATH, fname );
	return;
}

//------------------------------------------------
// ファイルを閉じる
//------------------------------------------------
void CIni::close( void )
{
	msFilename[0] = '\0';
	return;
}

//------------------------------------------------
// [get] 整数値
//------------------------------------------------
int CIni::getInt( const char *sec, const char *key, int defval )
{
	return GetPrivateProfileIntA( sec, key, defval, msFilename );
}

//------------------------------------------------
// [get] 文字列
//------------------------------------------------
const char * CIni::getString( const char *sec, const char *key, const char *defval, size_t size )
{
	if ( size ) reserve( size );
	
	GetPrivateProfileStringA( sec, key, defval, msBuf, msizeBuf - 1, msFilename );
	return msBuf;
}

//------------------------------------------------
// [set] 整数値
//------------------------------------------------
void CIni::setInt( const char *sec, const char *key, int val, int radix )
{
	_itoa_s( val, msBuf, msizeBuf, radix );
	
	WritePrivateProfileStringA( sec, key, msBuf, msFilename );
	return;
}

//------------------------------------------------
// [set] 文字列
//------------------------------------------------
void CIni::setString( const char *sec, const char *key, const char *val )
{
	sprintf_s( msBuf, msizeBuf, "\"%s\"", val );
	
	WritePrivateProfileStringA( sec, key, msBuf, msFilename );
	return;
}

//------------------------------------------------
// セクションを削除する
//------------------------------------------------
void CIni::removeSection( const char *sec )
{
	WritePrivateProfileStringA( sec, NULL, NULL, msFilename );
	return;
}

//------------------------------------------------
// キーを削除する
//------------------------------------------------
void CIni::removeKey( const char *sec, const char *key )
{
	WritePrivateProfileStringA( sec, key, NULL, msFilename );
	return;
}

//------------------------------------------------
// キーの有無
//------------------------------------------------
bool CIni::existsKey( const char *sec, const char *key )
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
	
	msizeBuf = newSize + 0x100;
	msBuf    = (char *)realloc( msBuf, msizeBuf * sizeof(char) );
	return;
}
