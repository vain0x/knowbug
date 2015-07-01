// strbuf.cpp header

#ifndef IG_STRBUF_H
#define IG_STRBUF_H

#define STRBUF_BLOCKSIZE 64
#define STRBUF_BLOCK_DEFAULT   0x0400
#define STRBUF_SEGMENT_DEFAULT 0x1000

#define STRINF_FLAG_NONE 0
#define STRINF_FLAG_USEINT 1
#define STRINF_FLAG_USEEXT 2

// STRBUF structure
struct STRBUF;
struct STRINF;

struct STRINF
{
	// String Data structure
	short flag;						// 使用フラグ(0=none/other=busy)
	short exflag;					// 拡張フラグ(未使用)
	STRBUF* intptr;					// 自身のアドレス
	size_t size;					// 確保サイズ
	char* ptr;						// バッファポインタ (strbuf-ptr := sbAlloc() などが返すもの)
	STRBUF* extptr;					// 外部バッファポインタ(STRINF)
	void* opt;						// オプション(ユーザー定義用)
};

struct STRBUF
{
	// String Data structure
	STRINF inf;						// バッファ情報
	char data[STRBUF_BLOCKSIZE];	// 内部バッファ
};

//##############################################################################
//                公開関数群
//##############################################################################
void sbInit(void);
void sbBye(void);

char* sbAlloc( size_t size );
char* sbAllocClear( size_t size );
void sbFree( void* ptr );
char* sbExpand( void* ptr, size_t size );
char* sbExpandClear( void* ptr, size_t size );
STRINF* sbGetSTRINF( void* ptr );

void sbCopy( char** ptr, const void* data, size_t size );
void sbStrCopy( char** ptr, const char* str );
void sbCat( char** ptr, const void* data, size_t size, int mode = 0 );
void sbStrCat( char** ptr, const char* str );

void* sbGetOption( char* ptr );
void sbSetOption( char* ptr, void* option );
void sbInfo( char* ptr );

#endif
