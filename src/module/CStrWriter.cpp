
#include <cassert>
#include <cstring>
#include "strf.h"

#include "CStrWriter.h"
#include "CStrBuf.h"

char const* const CStructedStrWriter::stc_strUnused =
#ifdef _DEBUG
	"[THIS STRING MUSTN'T APPEAR]"
#else
	nullptr
#endif
;

std::string const& CStrWriter::get() const { return buf_->get(); }

//------------------------------------------------
// 連結
//------------------------------------------------
void CStrWriter::cat(char const* s)
{
	buf_->append(s);
}

void CStrWriter::catCrlf()
{
	cat("\r\n");
}

//------------------------------------------------
// メモリダンプ文字列の連結
// 
// @ 最後の行に改行を挿入しない。
//------------------------------------------------
void CStrWriter::catDumpImpl( void const* data, size_t size )
{
	static size_t const stc_bytesPerLine = 0x10;
	auto const mem = static_cast<unsigned char const*>(data);
	size_t idx = 0;
	while ( idx < size ) {
		if ( idx != 0 ) catCrlf(); //delimiter

		fmt::MemoryWriter row;
		row << fmt::pad(fmt::hexu(idx), 4, '0');
		for ( size_t i = 0; (i < stc_bytesPerLine && idx < size); ++i, ++idx ) {
			row << ' ' << fmt::pad(fmt::hexu(mem[idx]), 2, '0');
		}
		cat(row.c_str());
	}
}

void CStrWriter::catDump(void const* data, size_t bufsize)
{
	assert(bufsize == 0 || data);

	static size_t const stc_maxsize = 0x10000;
	size_t size = bufsize;

	if ( size > stc_maxsize ) {
		catln(strf("全%d[byte]の内、%d[byte]のみダンプします。", bufsize, stc_maxsize));
		size = stc_maxsize;
	}

	catln("dump  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
	catln("----------------------------------------------------");
	catDumpImpl(data, size);
}
