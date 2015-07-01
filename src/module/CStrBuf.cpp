// 文字列バッファ

#include <cassert>
#include <cstring>
#include "CStrBuf.h"

#include "strf.h"

char const* const CStrBuf::stc_warning = "(too long)";

//**********************************************************
//    構築と解体
//**********************************************************
//------------------------------------------------
// 構築
//------------------------------------------------
CStrBuf::CStrBuf(size_t lenLimit)
	: mpBuf( new string )
	, mlenLimit(lenLimit)
{ }

CStrBuf::CStrBuf()
	: CStrBuf(0)
{
	setLenLimit(-1);
}

//------------------------------------------------
// 解体
//------------------------------------------------
CStrBuf::~CStrBuf()
{
	delete mpBuf; mpBuf = nullptr;
}

//**********************************************************
//    設定
//**********************************************************
//------------------------------------------------
// 最短文字列長を設定する
//------------------------------------------------
void CStrBuf::setLenLimit( int lenLimit )
{
	mlenLimit = lenLimit;
}

//------------------------------------------------
// 確保
//------------------------------------------------
void CStrBuf::reserve( size_t additionalCap )
{
	if ( mlenLimit >= 0 ) {
		additionalCap = std::min(additionalCap, static_cast<size_t>(mlenLimit) + 1);
	}
	mpBuf->reserve( mpBuf->capacity() + additionalCap );
	return;
}

//**********************************************************
//    文字列の連結
//**********************************************************
//------------------------------------------------
// 単純な連結
//------------------------------------------------
void CStrBuf::cat( char const* s )
{
	if ( mlenLimit == 0 ) return;
	
	if ( mlenLimit >= 0 ) {
		size_t const len = std::strlen(s);
		
		if ( static_cast<int>(len) > mlenLimit ) {
			mpBuf->append( s, mlenLimit - stc_warning_length );
			mpBuf->append( stc_warning );
			mlenLimit = 0;
		} else {
			mpBuf->append( s );
			mlenLimit -= len;
		}
	} else {
		mpBuf->append( s );
	}
	return;
}

//------------------------------------------------
// 改行の連結
//------------------------------------------------
void CStrBuf::catCrlf()
{
	cat( "\r\n" );
}

//------------------------------------------------
// メモリダンプ文字列の連結
// 
// @ 最後の行に改行を挿入しない。
//------------------------------------------------
void CStrBuf::catDumpImpl( void const* pBuf, size_t bufsize )
{
	static size_t const stc_bytesPerLine = 0x10;

	assert(pBuf);

	auto const mem = static_cast<unsigned char const*>(pBuf);
	char tline[(4 + 3 * stc_bytesPerLine) + 1];
	size_t len = 0;
	size_t idx = 0;
	while ( idx < bufsize ) {
		if ( len != 0 ) catCrlf();		// 1回目は出力しない
		
		len = std::sprintf(tline, "%04X", idx);
		for ( size_t i = 0; (i < stc_bytesPerLine && idx < bufsize); ++i, ++idx ) {
			len += std::sprintf(&tline[len], " %02X", mem[idx]);
		}
		cat( tline );
	}
	return;
}

void CStrBuf::catDump(void const* data, size_t bufsize)
{
	if (data == nullptr || bufsize == 0) return;

	static const size_t stc_maxsize = 0x10000;
	size_t size = bufsize;

	if ( size > stc_maxsize ) {
		catln(strf("全%d[byte]の内、%d[byte]のみダンプします。", bufsize, stc_maxsize));
		size = stc_maxsize;
	}

	catln("dump  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
	catln("----------------------------------------------------");
	catDumpImpl(data, size);
	return;
}

