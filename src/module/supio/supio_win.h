// Support IO

#ifndef IG_SUPIO_WIN_H
#define IG_SUPIO_WIN_H

#include <cctype>

//**********************************************************
//        メモリ管理
//**********************************************************
char* mem_ini( int size );
void mem_bye( void* ptr );
char* mem_alloc( void* base, int newsize, int oldsize );
int mem_load( char const* fname, void* mem, int msize );
int mem_save( char const* fname, void const* mem, int msize );
int mem_save( char const* fname, void const* mem, int msize, int seekofs );

//**********************************************************
//        文字列操作
//**********************************************************
//void prtini( char* mes );
//void prt( char* mes );

int is_sjis_char_head( const unsigned char* str, int pos );

bool tstrcmp( char const* str1, char const* str2 );
void strcase( char* str );
void strcpy_case( char* str, char* str2 );
void strcpy2( char* dest, char const* src, size_t size );
char* strchr2( char* target, char code );
int   strcat2( char* str1, char* str2 );
char* strstr2( char* target, char* src );

void addext( char* st, char const* exstr );
void cutext( char* st );
void cutlast( char* st );
void cutlast2( char* st );
void getpath( char const* src, char* outbuf, int p2 );

int atoi_allow_overflow( char const* s );
char* to_hsp_string_literal( char const* src );

//**********************************************************
//        ファイル操作
//**********************************************************
int filecopy( char const* fname, char const* sname );

void ExecFile( char* stmp, char* ps, int mode );
void dirinfo( char* p, int id );

int delfile( char* name );
int makedir( char* name );
int changedir( char* name );
int dirlist( char* fname, char** target, int p3 );

void strsp_ini(void);
int strsp_getptr(void);
int strsp_get( char* srcstr, char* dststr, char splitchr, int len );
int GetLimit( int num, int min, int max );
void CutLastChr( char* p, char code );
char* strsp_cmds( char* srcstr );
int htoi( char const* str );

#ifdef HSP3IMP
int SecurityCheck( char* name );
#endif

//**********************************************************
//        strtrim
//**********************************************************
char* strchr3( char* target, int code, int sw, char** findptr );
void TrimCode( char* p, int code );
void TrimCodeL( char* p, int code );
void TrimCodeR( char* p, int code );

//**********************************************************
//        Dialog
//**********************************************************
void Alert( char const* mes );
void AlertV( char const* mes, int val );
void Alertf( char const* format, ... );

//**********************************************************
//        その他
//**********************************************************
int gettime( int index );

//**********************************************************
//        インライン関数
//**********************************************************
//------------------------------------------------
// SJISの Lead/Trail Byte の範囲内か
//------------------------------------------------
inline bool isLeadByte(unsigned char c)
{
	return (0x81 <= c && c <= 0x9F)
		|| (0xE0 <= c && c <= 0xFC);
}

inline bool isTrailByte(unsigned char c)
{
	return (0x40 <= c && c <= 0x7E)
		|| (0x80 <= c && c <= 0xFC);
}

inline int issjisleadbyte ( unsigned char c ) { return isLeadByte( c ); }
inline int issjistrailbyte( unsigned char c ) { return isTrailByte( c ); }
inline bool isSJIS1st     ( unsigned char c ) { return isLeadByte( c ); }
inline bool isSJIS2nd     ( unsigned char c ) { return isTrailByte( c ); }

//------------------------------------------------
// 文字は記号か
// 
// @ 文字 '@', '_' も記号扱いする。
// @	記号が識別子に含まれない文字とは限らない。
//------------------------------------------------
inline bool isSignChar(unsigned char c)
{
	return ( c < 0x30 )
		|| (0x3A <= c && c <= 0x40)
		|| (0x5B <= c && c <= 0x60)
		|| (0x7B <= c && c <= 0x7F);
}

//------------------------------------------------
// clhspの識別子に使用できる文字か
// 
// @ 文字 '@' も識別子の一部とする。
//------------------------------------------------
inline bool isIdentTop(unsigned char c)
{
	return ('a' <= c && c <= 'z')
		|| ('A' <= c && c <= 'Z')
		|| c == '_'
		|| isSJIS1st( c )
	;
}

inline bool isIdent(unsigned char c)
{
	return (isIdentTop(c) || std::isdigit(c) || c == '@');
}

#endif
