// clhsp - Basic I/O and Message Buffer Support

// @ Window, Console 両方に対応

#include "hsp3config.h"

#ifdef HSPWIN
# define USE_WINDOWS_API		// WINDOWS APIを使用する
#endif

#ifdef USE_WINDOWS_API
# include <windows.h>
# include <shlobj.h>
# include "strbuf.h"			// dirlist で使用する
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <direct.h>
#include <ctype.h>

#include "supio_win.h"

//**********************************************************
//        メモリ管理
//**********************************************************
//------------------------------------------------
// 指定サイズのメモリを確保して返す
//------------------------------------------------
char* mem_ini( int size )
{
	return (char*)malloc(size);
}

//------------------------------------------------
// mem_ini で確保したメモリを解放する
//------------------------------------------------
void mem_bye( void* ptr )
{
	free( ptr );
	return;
}

//------------------------------------------------
// ファイル -> メモリ
//------------------------------------------------
int mem_load( char const* fname, void* mem, int msize )
{
	FILE *fp = fopen( fname, "rb" );
	if ( fp == NULL ) return -1;
	
	int flen = (int)fread( mem, 1, msize, fp );
	fclose(fp);
	return flen;
}

//------------------------------------------------
// メモリ -> ファイル
//------------------------------------------------
int mem_save( char const* fname, void const* mem, int msize )
{
	FILE *fp = fopen( fname, "wb" );
	if ( fp == NULL ) return -1;
	
	int flen = static_cast<int>( fwrite( mem, 1, msize, fp ) );
	fclose(fp);
	return flen;
}

int mem_save( char const* fname, void const* mem, int msize, int seekofs )
{
	FILE *fp;
	int flen;

	if ( seekofs < 0 ) {
		fp = fopen( fname, "wb" );
	} else {
		fp = fopen(fname,"r+b");
	}
	if ( fp == NULL ) return -1;
	
	if ( seekofs >= 0 ) fseek( fp, seekofs, SEEK_SET );
	
	flen = static_cast<int>( fwrite( mem, 1, msize, fp ) );
	fclose(fp);
	return flen;
}

//**********************************************************
//        Memory Manager
//**********************************************************
//------------------------------------------------
// メモリ確保
//------------------------------------------------
char* mem_alloc( void* base, int newsize, int oldsize )
{
	char* p;
	
	if ( base == NULL ) {
		p = (char*)calloc( newsize, 1 );
		
	} else {
		if ( newsize <= oldsize ) return (char*)base;
		p = (char*)calloc( newsize, 1 );
		memcpy( p, base, oldsize );
		free( base );
	}
	return p;
}

//**********************************************************
//        ファイル操作
//**********************************************************
//------------------------------------------------
// ファイル複写
//------------------------------------------------
int filecopy( char const* fname, char const* sname )
{
	int flen;
	int retval = 1;
	int max    = 0x8000;
	char* mem  = mem_ini( max );
	
	{
		FILE * fpSrc = fopen(fname, "rb"); if ( fpSrc == NULL ) goto LErrorReturn;
		FILE * fpDst = fopen(sname, "wb"); if ( fpDst == NULL ) goto LErrorReturn;
		
		for (;;) {
			flen = (int)fread( mem, 1, max, fpSrc );
			if ( flen == 0 ) break;
			
			fwrite( mem, 1, flen, fpDst );
			if ( flen < max ) break;
		}
		fclose(fpDst);
		fclose(fpSrc);
		retval = 0;
	}
	
LErrorReturn:
	mem_bye(mem);
	return retval;
}

//------------------------------------------------
// ファイル削除
//------------------------------------------------
int delfile( char* name )
{
#ifdef HSPWIN
	return DeleteFile( name );
#else
	return 0;
#endif
}

//------------------------------------------------
// ディレクトリ作成
//------------------------------------------------
int makedir( char* name )
{
#ifdef HSPWIN
	return _mkdir( name );
#else
	return 0;
#endif
}

//------------------------------------------------
// ディレクトリ移動
//------------------------------------------------
int changedir( char* name )
{
#ifdef HSPWIN
	return _chdir( name );
#else
	return 0;
#endif
}

//------------------------------------------------
// ディレクトリリスト
// 
// @prm target : sb-ptr
//------------------------------------------------
int dirlist( char* fname, char** target, int p3 )
{
#ifdef HSPWIN
	char* p;
	int fl;
	int stat_main;
	HANDLE sh;
	WIN32_FIND_DATA fd;
	DWORD fmask;
	BOOL ff;
	
	fmask = 0;
	if ( p3 & 1 ) fmask |= FILE_ATTRIBUTE_DIRECTORY;
	if ( p3 & 2 ) fmask |= FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
	
	stat_main = 0;
	
	sh = FindFirstFile( fname, &fd );
	if ( sh == INVALID_HANDLE_VALUE ) return 0;
	
	for (;;) {
		ff = ( fd.dwFileAttributes & fmask ) > 0;
		if ( !( p3 & 4 ) ) ff = !ff;
		
		if ( ff ) {
			p  = fd.cFileName;
			fl = 1;
			if ( *p == 0 ) fl = 0;			// 空行を除外
			if ( *p == '.' ) {				// '.', '..'を除外
				if ( p[1] == '\0' ) fl = 0;
				if ( p[1] == '.' && p[2] == '\0' ) fl = 0;
			}
			if ( fl ) {
				stat_main ++;
				sbStrCat( target, p );
				sbStrCat( target, "\r\n" );
			}
		}
		if ( !FindNextFile(sh, &fd) ) break;
	}
	FindClose(sh);
	return stat_main;
#else
	return 0;
#endif
}

//**********************************************************
//        文字列操作
//**********************************************************
//------------------------------------------------
// 文字列比較
//------------------------------------------------
bool tstrcmp( char const* str1, char const* str2 )
{
	int i = 0;
	char c;
	
	for (;;) {
		c = str1[i];
		if ( c != str2[i] ) return false;
		if ( c == '\0' ) break;
		i ++;
	}
	
	return true;
}

//------------------------------------------------
// 文字列を標準化する
// 
// @ 大文字を小文字に直す。
// @prm str : 文字列へのポインタ (直接変更される)
//------------------------------------------------
void strcase( char* str )
{
#ifdef NOUSE_CASE
	unsigned char c;
	unsigned char* p = (unsigned char*)str;
	for (;;) {
		c = *p;
		if ( c == '\0' ) break;
		if ( c >= 0x80 ) {
			*p ++;
			c = *(p ++);
			if ( c == '\0' ) break;
		} else {
			*(p ++) = tolower(c);
		}
	}
#endif
	return;
}

//------------------------------------------------
// 文字列を標準化しつつ複写する
// 
// @prm sSrc : 複写元。こちらも標準化される。
// @prm sDst : 複写先。
//------------------------------------------------
void strcpy_case( char* prm_sSrc, char* prm_sDst )
{
	unsigned char c;
	unsigned char* sSrc = (unsigned char*)prm_sSrc;
	unsigned char* sDst = (unsigned char*)prm_sDst;
	
	for (;;) {
		c = *sSrc;
		
		if ( c == '\0' ) break;
		if ( c >= 0x80 ) {
			*sSrc ++;
			*(sDst ++) = c;
			c = *(sSrc ++);
			if ( c == 0 ) break;
			*(sDst ++) = c;
		} else {
#ifdef NOUSE_CASE
			c = tolower( c );
#endif
			*(sSrc ++) = //
			*(sDst ++) = c;
		}
	}
	
	*sDst = '\0';
	return;
}

//------------------------------------------------
// 文字列を複写する
//------------------------------------------------
static size_t strcpy2( char* str1, char* str2 )
{
	char* p   = str1;
	char* src = str2;
	
	while ( *src != '\0' ) {
		*(p ++) = *(src ++);
	}
	
	*(p ++) = '\0';
	return (p - str1);
}

//------------------------------------------------
// 指定サイズの文字列を複写する
// 
// @tricky
//------------------------------------------------
void strcpy2( char* dst, char const* src, size_t size )
{
	if ( size == 0 ) return;
	
	char       *d = dst;
	char const* s = src;
	size_t      n = size;
	while ( -- n ) {
		if ( ( *(d ++) = *(s ++) ) == '\0' ) {
			return;
		}
	}
	*d = '\0';
	return;
}

//------------------------------------------------
// 文字列連結
// 
// @result: 結合後の長さ
//------------------------------------------------
int strcat2( char* str1, char* str2 )
{
	char* src;
	src = str1;
	
	while ( *src != '\0' ) src ++;
	
	int i = static_cast<int>(src - str1);
	return static_cast<int>( strcpy2(src, str2) + i );
}

//------------------------------------------------
// 文字列検索
// 
// @ 全角文字に対応。
//------------------------------------------------
char* strstr2( char* target, char* src )
{
	unsigned char* p;
	unsigned char* s;
	unsigned char* p2;
	unsigned char a1;
	unsigned char a2;
	unsigned char a3;
	
	p = (unsigned char*)target;
	if ( *src == '\0' || *target == '\0' ) return NULL;
	
	for (;;) {
		a1 = *p;
		if ( a1 == '\0' ) break;
		p2 = p;
		s = (unsigned char*)src;
		for (;;) {
			a2 = *(s  ++); if ( a2 == 0 ) return (char*)p;
			a3 = *(p2 ++); if ( a3 == 0 ) break;
			if ( a2 != a3 ) break;
		}
		p ++;							// 検索位置を移動
	//*
		if ( isSJIS1st(a1) ) p ++;
	/*/
		if (a1>=129) {					// 全角文字チェック
			if ((a1<=159)||(a1>=224)) p++;
		}
	//*/
	}
	return NULL;
}

//------------------------------------------------
// getpath 処理
//------------------------------------------------
void getpath( char const* src, char* outbuf, int p2 )
{
#ifdef USE_WINDOWS_API
	char* p;
	char stmp   [_MAX_PATH];
	char p_drive[_MAX_PATH];
	char p_dir  [_MAX_DIR];
	char p_fname[_MAX_FNAME];
	char p_ext  [_MAX_EXT];
	
	p = outbuf;
	strcpy( stmp, src );
	if ( p2 & 16 ) strcase( stmp );
	_splitpath( stmp, p_drive, p_dir, p_fname, p_ext );
	strcat( p_drive, p_dir );
	if ( p2 & 8 ) {
		strcpy( stmp, p_fname );
		strcat( stmp, p_ext );
	} else if ( p2 & 32 ) {
		strcpy( stmp, p_drive );
	}
	switch ( p2 & 7 ) {
		case 1:			// Name only ( without ext )
			stmp[ strlen(stmp) - strlen(p_ext) ] = '\0';
			strcpy( p, stmp );
			break;
		case 2:			// Ext only
			strcpy( p, p_ext );
			break;
		default:		// Direct Copy
			strcpy( p, stmp );
			break;
	}
#else
	*outbuf = '\0';
#endif
	return;
}

//**********************************************************
//        文字列操作 ( ファイル名 )
//**********************************************************
//------------------------------------------------
// 拡張子を探す
//------------------------------------------------
static int findext( char const*  st )
{
	int r = -1;
	bool bTrail ( false );
	
	for ( int i = 0; st[i] != '\0'; ++ i ) {
		if ( bTrail ) {
			bTrail = false;
		} else {
			if ( st[i] == '.' ) {
				r = i;
			} else if ( st[i] == '\\' ) {
				r = -1;
			}
			bTrail = ( isSJIS1st( st[i] ) != FALSE );
		}
	}
	return r;
}

//------------------------------------------------
// 拡張子(extension)を付加する
// 
// @ すでに拡張子があれば付加しない。
//------------------------------------------------
void addext( char* st, char const* exstr )
{
	int idx = findext( st );
	
	if ( idx < 0 ) {
		strcat( st, "." );
		strcat( st, exstr );
	}
	return;
}

//------------------------------------------------
// 拡張子を取り除く
//------------------------------------------------
void cutext( char*  st )
{
	int idx ( findext(st) );
	if ( idx >= 0 ) st[idx] = '\0';
	return;
}

//------------------------------------------------
// cut last characters
//------------------------------------------------
void cutlast( char* st )
{
	int idx ( 0 );
	unsigned char c;
	
	for (;;) {
		c = st[idx];
		if ( c <= 32 ) break;	// 印字不可能文字
		st[idx] = tolower(c);
		idx ++;
	}
	st[idx] = '\0';
	return;
}

//------------------------------------------------
// cut last characters
//------------------------------------------------
void cutlast2( char* st )
{
	int a;
	char c;
	char ts[256];

	strcpy(ts, st);
	a = 0;
	for (;;) {
		c = ts[a];
		if ( c <= 32 ) break;
		ts[a] = tolower(c);
		a ++;
	}
	ts[a] = '\0';
	
	for (;;) {
		a --;
		c = ts[a];
		if ( c == '\\' ) { a ++; break; }
		if ( a == '\0' ) break;
	}
	strcpy(st, ts + a);
	return;
}

//------------------------------------------------
// str中最後のcode位置を探す(全角対応版)
//------------------------------------------------
char* strchr2( char* target, char code )
{
	unsigned char a1;
	unsigned char* p = (unsigned char*)target;
	char* res = NULL;
	
	for (;;) {
		a1 = *p;
		if ( a1 == '\0' ) break;
		if ( a1 == code ) res = (char*)p;
		p ++;							// 検索位置を移動
	//*
		if ( isSJIS1st(a1) ) p ++;
	/*/
		if ( a1 >= 129 ) {				// 全角文字チェック
			if ( a1 <= 159 || a1 >= 224 ) p ++;
		}
	//*/
	}
	return res;
}

//------------------------------------------------
// 先頭(lead)バイトかどうか
// 
// @ Shift_JIS文字列のposバイト目が文字の先頭バイトであるか
// @ マルチバイト文字の後続バイトなら0、それ以外なら1を返す
//------------------------------------------------
int is_sjis_char_head( const unsigned char* str, int pos )
{
	int result = 1;
	while ( pos != 0 && issjisleadbyte(str[-- pos]) ) {
		result = !result;
	}
	return result;
}

//------------------------------------------------
// 文字列をHSPの文字列リテラル形式にする
// 
// @! HSPの文字列リテラルで表せない文字はそのまま出力される (CRなど)
// @! 戻り値は、呼び出し側が free すること (free義務)。
// @result: 文字列へのポインタ + free 義務
//------------------------------------------------
char* to_hsp_string_literal( char const* src )
{
	typedef unsigned char uchar;
	
	size_t length = 2;
	uchar const* s = (uchar*)src;
	
	for (;;) {
		uchar c = *s;
		
		if ( c == '\0' ) break;
		
		switch ( c ) {
			case '\r':
				if ( *(s + 1) == '\n' ) {
					s ++;
				}
				// fall-through
				
			case '\t':
			case '"':
			case '\\':
				length += 2;
				break;
				
			default:
				length ++;
		}
		
		if ( isLeadByte(c) && *(s + 1) != '\0' ) {
			length ++;
			s += 2;
		} else {
			s ++;
		}
	}
	char* dest = (char*)malloc( length + 1 );
	if ( dest == NULL ) return dest;
	s = (uchar*)src;
	uchar* d = (uchar*)dest;
	*(d ++) = '"';
	
	for (;;) {
		uchar c = *s;
		if ( c == '\0' ) break;
		switch (c) {
			case '\t':
				*(d ++) = '\\';
				*(d ++) = 't';
				break;
			case '\r':
				*(d ++) = '\\';
				if ( *(s + 1) == '\n') {
					*(d ++) = 'n';
					s ++;
				} else {
					*(d ++) = 'r';
				}
				break;
			case '"':
				*(d ++) = '\\';
				*(d ++) = '"';
				break;
			case '\\':
				*(d ++) = '\\';
				*(d ++) = '\\';
				break;
			default:
				*(d ++) = c;
				if( isLeadByte(c) && *(s + 1) != '\0' ) {
					*(d ++) = *(++ s);
				}
		}
		s ++;
	}
	*(d ++) = '"';
	*d      = '\0';
	return dest;
}

//------------------------------------------------
// 最後の '\\' を取り除く
//------------------------------------------------
void CutLastChr( char* p, char code )
{
	char* ss;
	char* ss2;
	int i;
	ss = strchr2( p, '\\' );
	if ( ss != NULL ) {
		i   = static_cast<int>( strlen(p) );
		ss2 = p + i - 1;
		if ( ( i > 3 ) && ( ss == ss2 ) ) *ss = '\0';
	}
	return;
}

//------------------------------------------------
// Over-flow をチェックしない atoi
//------------------------------------------------
int atoi_allow_overflow( char const* s )
{
	int result = 0;
	while ( isdigit(*s) ) {
		result = result * 10 + (*s - '0');
		s ++;
	}
	return result;
}

int GetLimit( int num, int min, int max )
{
	if ( num > max ) return max;
	if ( num < min ) return min;
	return num;
}

//------------------------------------------------
// 16進数を10進数に変換する
//------------------------------------------------
static int htoi_sub( char c );

int htoi( char const* s )
{
	int idx  = 0;
	int conv = 0;
	
	for (;;) {
		char c = s[idx];
		idx ++;
		if ( !isxdigit(c) ) break;
		conv = (conv * 0x10) + htoi_sub(c);
	}
	
	return conv;
}

static int htoi_sub( char c )
{
	c = tolower(c);
	if ( '0' <= c && c <= '9' ) return c - '0';
	if ( 'a' <= c && c <= 'f' ) return c - 'a' + 10;
	return 0;
}

//**********************************************************
//        strtrim
//**********************************************************
//------------------------------------------------
// 文字列中のcode位置を探す (2Bコード、全角対応版)
// 
// @prm findptr:
// @	(sw == 0) => findptr := 最後に見つかったcode位置
// @	(sw == 1) => findptr := 最初に見つかったcode位置
// @	(sw == 2) => findptr := 最初に見つかったcode位置 (最初の文字のみ検索)
// @result: 次の文字にあたる位置
//------------------------------------------------
char* strchr3( char* target, int code, int sw, char** findptr )
{
	unsigned char* p;
	unsigned char a1;
	unsigned char code1;
	unsigned char code2;
	char* res;
	char* pres;
	
	p     = (unsigned char*)target;
	code1 = (unsigned char)(code & 0xFF);
	code2 = (unsigned char)(code >> 8);
	
	res      = NULL;
	pres     = NULL;
	*findptr = NULL;
	
	for (;;) {
		a1 = *p;
		if ( a1 == 0 ) break;
		if ( a1 == code1 ) {
			if ( a1 < 129 ) {
				res = (char*)p;
			} else {
				if ( ( a1 <= 159 ) || ( a1 >= 224 ) ) {
					if ( p[1] == code2 ) {
						res = (char*)p;
					}
				} else {
					res = (char*)p;
				}
			}
		}
		p ++;							// 検索位置を移動
	//*
		if ( isSJIS1st(a1) ) ++ p;		// 全角文字
	/*/
		if ( a1 >= 0x81 ) {				// 全角文字チェック
			if ( ( a1 <= 159 ) || ( a1 >= 224 ) ) p ++;
		}
	//*/
		if ( res != NULL ) {
			*findptr = res;
			pres     = (char*)p;
			res      = NULL;
		}
		
		switch( sw ) {
			case 1:
				if ( *findptr != NULL ) return (char*)p;
				break;
			case 2:
				return (char*)p;
		}
	}
	return pres;
}

//------------------------------------------------
// 最後のcodeを取り除く
//------------------------------------------------
void TrimCodeR( char* p, int code )
{
	char* ss;
	char* ss2;
	char* sslast;
	int i;
	for (;;) {
		i      = (int)strlen( p );
		sslast = p + i;
		ss     = strchr3( p, code, 0, &ss2 );
		if ( ss2 == NULL ) break;
		if ( ss != sslast ) break;
		*ss2 = 0;
	}
	return;
}

//------------------------------------------------
// すべてのcodeを取り除く
//------------------------------------------------
void TrimCode( char* p, int code )
{
	char* ss;
	char* ss2;
	
	for (;;) {
		ss = strchr3( p, code, 1, &ss2 );
		if ( ss2 == NULL ) break;
		strcpy( ss2, ss );
	}
	return;
}

//------------------------------------------------
// 最初のcodeを取り除く
//------------------------------------------------
void TrimCodeL( char* p, int code )
{
	char* ss;
	char* ss2;
	for (;;) {
		ss = strchr3( p, code, 2, &ss2 );
		if ( ss2 == NULL ) break;
		strcpy( ss2, ss );
	}
	return;
}

//**********************************************************
//        dirinfo
//**********************************************************
//------------------------------------------------
// dirinfo 処理
// 
// @ stmp に設定される
//------------------------------------------------
void dirinfo( char* p, int id )
{
#ifdef USE_WINDOWS_API
	char fname[_MAX_PATH + 1];
	
	switch( id ) {
		case 0:				// カレント・ディレクトリ
			_getcwd( p, _MAX_PATH );
			break;
		case 1:				// 実行ファイルがあるディレクトリ
			GetModuleFileName( NULL, fname, _MAX_PATH );
			getpath( fname, p, 32 );
			break;
		case 2:				// Windowsディレクトリ
			GetWindowsDirectory( p, _MAX_PATH );
			break;
		case 3:				// Windowsのシステムディレクトリ
			GetSystemDirectory( p, _MAX_PATH );
			break;
		default:
			if ( id & 0x10000 ) {
				SHGetSpecialFolderPath( NULL, p, id & 0xFFFF, FALSE );
				break;
			}
			*p = 0;
			return;
	}

	// 最後の '\\' を取り除く
	CutLastChr( p, '\\' );
#else
	*p = 0;
#endif
}

//**********************************************************
//        string-splitting
//**********************************************************

static int splc = 0;	// split pointer

void strsp_ini(void)
{
	splc = 0;
}

int strsp_getptr(void)
{
	return splc;
}

int strsp_get( char* srcstr, char* dststr, char splitchr, int len )
{
	//		split string with parameters
	//
	
	unsigned char c;
	unsigned char c2;
	int  iDst  = 0;
	bool bSjis = false;
	
	for (;;) {
		c = srcstr[splc];
		if ( c == 0 ) break;
		splc ++;
	//*
		if ( isLeadByte(c) ) bSjis = true;
	/*/
		if ( c >= 0x81 ) if ( c < 0xa0 ) bSjis ++;
		if ( c >= 0xe0) bSjis ++;
	//*/
		
		if ( c == splitchr ) break;
		if ( c == '\r' ) {
			c2 = srcstr[splc];
			if ( c2 == '\n' ) splc ++;
			break;
		}
		dststr[iDst ++] = c;
		
		if ( bSjis ) {
			dststr[iDst ++] = srcstr[splc ++];
			bSjis           = false;
		}
		
		if ( iDst >= len ) break;
	}
	
	dststr[iDst] = '\0';
	return static_cast<int>( c );
}

char* strsp_cmds( char* srcstr )
{
	//		Skip 1parameter from command line
	//
	bool bInQuote = false;
	char* cmdchk  = srcstr;
	char c;
	
	for (;;) {
		c = *cmdchk;
		if ( c == '\0' ) break;
		cmdchk ++;
		if ( c == ' ' && !bInQuote ) break;
		if ( c == '"' ) bInQuote = !bInQuote;
	}
	return cmdchk;
}

//**********************************************************
//        Windows Debug Support
//**********************************************************
#ifdef USE_WINDOWS_API

void Alert( char const* mes )
{
	MessageBox( NULL, mes, "error", MB_ICONINFORMATION | MB_OK );
	return;
}

void AlertV( char const* mes, int val )
{
	char ss[128];
	sprintf( ss, "%s%d", mes, val );
	MessageBox( NULL, ss, "error", MB_ICONINFORMATION | MB_OK );
	return;
}

void Alertf( char const* format, ... )
{
	char textbf[1024];
	va_list args;
	va_start(args, format);
	vsprintf( textbf, format, args );
	va_end(args);
	MessageBox( NULL, textbf, "error", MB_ICONINFORMATION | MB_OK );
	return;
}

#endif

//**********************************************************
//        その他
//**********************************************************
//------------------------------------------------
// 現在時刻の一部を取得する
// 
// @prm index:
// @	0 => year
// @	1 => month ( 1ベース )
// @	2 => day of weeek (曜日番号、0 が日曜日)
// @	3 => hour
// @	4 => minute
// @	5 => second
// @	6 => milli-second
//------------------------------------------------
int gettime( int index )
{
#ifdef HSPWIN
	SYSTEMTIME st;
	GetLocalTime( &st );
	short* p ( reinterpret_cast<short*>( &st ) );
	return static_cast<int>( p[index] );
#else
	return 0;
#endif
}
