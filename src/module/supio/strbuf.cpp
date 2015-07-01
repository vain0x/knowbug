
//
//	HSP3 string support
//	(おおらかなメモリ管理をするバッファマネージャー)
//	(sbAllocでSTRBUF_BLOCKSIZEのバッファを確保します)
//	(あとはsbCopy,sbAddで自動的にバッファの再確保を行ないます)
//	onion software/onitama 2004/6
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "supio.h"
#include "strbuf.h"

#include "hspsdk/hsp3debug.h"

#define REALLOC realloc
#define MALLOC malloc
#define FREE free

/*------------------------------------------------------------*/
/*
		system data
*/
/*------------------------------------------------------------*/

struct SLOT
{
	STRBUF* mem;
	int len;
};

static SLOT* mem_sb;
static int str_blockcur;
static int slot_len;

static STRBUF* freelist;

// STRINF_FLAG_NONE のとき STRINF::extptr を free list の次のポインタに使う
#define STRINF_NEXT(inf) ((inf).extptr)
#define STRBUF_NEXT(buf) STRINF_NEXT((buf)->inf)

#define GET_INTINF(buf) (&((buf)->inf.intptr->inf))

/*------------------------------------------------------------*/
/*
		internal function
*/
/*------------------------------------------------------------*/

//------------------------------------------------
// 確保サイズの正規化
// 
// @ 0x100 単位で確保
// @default: (size + 0xFFF)  0xFFFFF000;	// 8K単位で確保
//------------------------------------------------
static inline size_t RegularlizeAllocSize( size_t size )
{
	return ( size + 0xFF & 0xFFFFFF00 );
}

//------------------------------------------------
// 新たな STRBUF を大量に確保する
// 
// @ 新しく SLOT を作る。
//------------------------------------------------
static void BlockPtrPrepare(void)
{
	STRBUF* sb;
	
	if ( str_blockcur == 0 ) {
		mem_sb = (SLOT *)MALLOC( sizeof(SLOT) );
	} else {
		mem_sb = (SLOT *)REALLOC( mem_sb, sizeof(SLOT) * ( str_blockcur + 1 ) );
	}
	
	sb = (STRBUF*)MALLOC( sizeof(STRBUF) * slot_len );
	if ( sb == NULL ) throw HSPERR_OUT_OF_MEMORY;
	STRBUF* p = sb;
	STRBUF* pend = p + slot_len;
	mem_sb[ str_blockcur ].mem = sb;
	mem_sb[ str_blockcur ].len = slot_len;
	str_blockcur ++;
	slot_len = (int)(slot_len * 1.8);
	
	while ( p < pend ) {
		p->inf.intptr  = p;
		p->inf.flag    = STRINF_FLAG_NONE;
		STRBUF_NEXT(p) = freelist;
		freelist = p;
		p ++;
	}
	return;
}

//------------------------------------------------
// 未使用の STRBUF へのポインタを得る
// 
// @ 空きエントリーブロックを探す
// @	探すというか、freelist が指しているけど。
//------------------------------------------------
static STRBUF* BlockEntry(void)
{
	if ( freelist == NULL ) {
		BlockPtrPrepare();
	}
	STRBUF* buf = freelist;
	freelist = STRBUF_NEXT(freelist);
	return buf;
}

//------------------------------------------------
// 文字列バッファを確保する
// 
// @ 64B 以下 ⇒ 64B確保。
//------------------------------------------------
static char* BlockAlloc( size_t size )
{
	char* p;
	STRBUF* st;
	STRBUF* st2;
	STRINF* inf;
	st  = BlockEntry();
	inf = &st->inf;
	
	if ( size <= STRBUF_BLOCKSIZE ) {
		inf->flag = STRINF_FLAG_USEINT;
		inf->size = STRBUF_BLOCKSIZE;
		p         = st->data;			// 配列へのポインタ
		inf->ptr  = p;
	} else {
		inf->flag   = STRINF_FLAG_USEEXT;
		inf->size   = size;
		st2         = (STRBUF*)MALLOC( sizeof(STRINF) + size );	// 拡張 STRBUF
		p           = st2->data;
		inf->extptr = st2;
		inf->ptr    = p;
		st2->inf    = *inf;
	}
	
	*(int*)p = 0;		// 先頭 4B は 0 にしておく。
//	return inf->ptr;
	return p;
}

//------------------------------------------------
// 拡張 STRBUF を解放する
//------------------------------------------------
static void FreeExtPtr( STRINF* inf )
{
	if ( inf->flag == STRINF_FLAG_USEEXT ) {
		FREE( inf->extptr );
	}
	return;
}

//------------------------------------------------
// 文字列バッファを解放する
// 
// @ 初期化して、未使用 STRBUF の連結リストに連結する。
//------------------------------------------------
static void BlockFree( STRINF* inf )
{
	FreeExtPtr( inf );
	STRINF_NEXT(*inf) = freelist;
	freelist = (STRBUF*)inf;
	inf->flag = STRINF_FLAG_NONE;
	return;
}

//------------------------------------------------
// 文字列バッファを拡張する
// 
// @ size が既存のサイズより小さい => do nothing
// @prm size : 拡張後のバッファサイズ
//------------------------------------------------
static char* BlockRealloc( STRBUF* st, size_t size )
{
	STRINF* inf = GET_INTINF(st);
	if ( size <= inf->size ) return inf->ptr;
	
	STRBUF* newst = (STRBUF*)MALLOC( sizeof(STRINF) + size );
	char*       p = newst->data;
	
	memcpy( p, inf->ptr, inf->size );
	
	FreeExtPtr( inf );		// 以前の extptr を解放する
	inf->size   = size;
	inf->flag   = STRINF_FLAG_USEEXT;
	inf->ptr    = p;
	inf->extptr = newst;
	
	newst->inf  = *inf;
	return p;
}

void BlockInfo( STRINF* inf )
{
	STRBUF* newst;
	if ( inf->flag == STRINF_FLAG_USEEXT ) {
		newst = (STRBUF*)inf->extptr;
	}
	return;
}

/*------------------------------------------------------------*/
/*
		interface
*/
/*------------------------------------------------------------*/

static bool stt_bInited(false);

//------------------------------------------------
// strbuf モジュールの初期化
// 
// @ グローバル変数の初期化
// @ 一つ目の SLOT の確保
//------------------------------------------------
void sbInit(void)
{
	if ( stt_bInited ) return;		// すでに初期化済み
	stt_bInited = true;
	
	str_blockcur = 0;
	freelist     = NULL;
	slot_len     = STRBUF_BLOCK_DEFAULT;
	BlockPtrPrepare();
	
	return;
}

//------------------------------------------------
// strbuf モジュールの後始末
// 
// @ すべての SLOT、STRBUF の解放
//------------------------------------------------
void sbBye(void)
{
	if ( !stt_bInited ) return;
	stt_bInited = false;
	
	for( int i = 0; i < str_blockcur; ++ i ) {
		STRBUF* mem  = mem_sb[i].mem;
		STRBUF* p    = mem;
		STRBUF* pend = p + mem_sb[i].len;
		while ( p < pend ) {
			FreeExtPtr( &p->inf );
			p ++;
		}
		FREE( mem );
	}
	FREE( mem_sb );
	mem_sb = NULL;
	return;
}

//------------------------------------------------
// 文字列バッファへのポインタから STRINF を得る
//------------------------------------------------
STRINF* sbGetSTRINF( void* ptr )
{
	return reinterpret_cast<STRINF*>( static_cast<char*>(ptr) - sizeof(STRINF) );
//	return (STRINF*)( reinterpret_cast<char*>(ptr) - sizeof(STRINF) );
}

//------------------------------------------------
// 文字列バッファを確保する
// 
// @ 初期化しない
// @result: strbuf-ptr
//------------------------------------------------
char* sbAlloc( size_t size )
{
	if ( size < STRBUF_BLOCKSIZE ) size = STRBUF_BLOCKSIZE;
	return BlockAlloc( size );
}

//------------------------------------------------
// 文字列バッファを確保＆初期化する
//------------------------------------------------
char* sbAllocClear( size_t size )
{
	char* p = sbAlloc( size );
	memset( p, 0x00, size );
	return p;
}

//------------------------------------------------
// 文字列バッファを解放する
// 
// @ (ptr == NULL) => do nothing
//------------------------------------------------
void sbFree( void* ptr )
{
	if ( ptr == NULL ) return;
	
	char*   p   = static_cast<char*>( ptr );
	STRBUF* st  = reinterpret_cast<STRBUF*>( p - sizeof(STRINF) );
	STRINF* inf = GET_INTINF(st);
	
	if ( p != inf->ptr ) return;
	
	BlockFree( inf );
	return;
}

//------------------------------------------------
// 文字列バッファを拡張する
//------------------------------------------------
char* sbExpand( void* ptr, size_t size )
{
	STRBUF* st = reinterpret_cast<STRBUF*>(static_cast<char*>(ptr)-sizeof(STRINF));
	return BlockRealloc( st, size );
}

char* sbExpandClear( void* ptr, size_t size )
{
	STRBUF* st = reinterpret_cast<STRBUF*>(static_cast<char*>(ptr)-sizeof(STRINF));
	size_t oldsize = st->inf.size;
	
	char* pNew = BlockRealloc( st, size );
	memset( &pNew[oldsize], 0x00, size );	// 新たな部分を 0 で埋める
	
	return pNew;
}

//------------------------------------------------
// 文字列を格納する
// 
// @ 前のデータは失われる。
// @ 自動拡張。
// @prm ppstr : strbuf-ptr へのポインタ( 変わるかもしれないため )
// @prm data  : 格納したい文字列へのポインタ。
// @            普通のバイナリデータで、strbuf-ptr でなくてもいい。
// @            const の扱いのはず、const が付いていないが……。
// @prm size  : 複写する長さ。
//------------------------------------------------
void sbCopy( char** pptr, void const* data, size_t size )
{
	char*  ptr = *pptr;
	STRBUF* st = reinterpret_cast<STRBUF*>( ptr - sizeof(STRINF) );
	size_t  sz = st->inf.size;
	char*    p = st->inf.ptr;
	
	if ( size > sz ) {
		p     = BlockRealloc( st, size );
		*pptr = p;
	}
	
	memcpy( p, data, size );
	return;
}

//------------------------------------------------
// データを連結する
// 
// @prm mode : 0 = normal, 1 = string
//------------------------------------------------
void sbCat( char** pptr, void const* data, size_t size, int mode )
{
	char*  ptr = *pptr;
	STRBUF* st = reinterpret_cast<STRBUF*>( ptr - sizeof(STRINF) );
	char*    p = st->inf.ptr;
	
	size_t sz, newsize;
	
	if ( mode ) {
		sz = static_cast<int>( strlen(p) );		// 文字列データ
	} else {
		sz = st->inf.size;		// 通常データ
	}
	
	newsize = sz + size;
	if ( newsize > (st->inf.size) ) {
		newsize = RegularlizeAllocSize( newsize ) ;
		//Alertf( "#Alloc%d", newsize );
		p = BlockRealloc( st, newsize );
		*pptr = p;
	}
	memcpy( p + sz, data, size );
	
	if ( mode ) {
		p[sz + size] = '\0';
	}
	return;
}

void sbStrCopy( char** ptr, char const* str )
{
	sbCopy( ptr, str, static_cast<int>(strlen(str)) + 1 );
}

void sbStrCat( char** ptr, char const* str )
{
	sbCat( ptr, str, strlen(str), 1 );
}

void* sbGetOption( char* ptr )
{
	STRBUF* st( (STRBUF*)( ptr - sizeof(STRINF) ) );
	return st->inf.opt;
}

void sbSetOption( char* ptr, void* option )
{
	STRBUF* st;
	STRINF* inf;
	
	st = reinterpret_cast<STRBUF*>( ptr - sizeof(STRINF) );
	st->inf.opt = option;
	inf = GET_INTINF(st);
	inf->opt = option;
	return;
}

/*
void sbInfo( char* ptr )
{
	STRBUF* st;
	st = ptr_cast<STRBUF*>( ptr - sizeof(STRINF) );
	Alertf( "size:%d (%x)",st->inf.size, st->inf.ptr );
}
*/
