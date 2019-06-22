
#include <cassert>
#include <cstring>
#include "strf.h"

#include "CStrWriter.h"
#include "CStrBuf.h"

char const* const CStructedStrWriter::stc_strUnused = "???";

auto CStrWriter::get() const -> std::string const& { return buf_->get(); }

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
	static auto const stc_bytesPerLine = size_t { 0x10 };
	auto const mem = static_cast<unsigned char const*>(data);
	auto idx = size_t { 0 };
	while ( idx < size ) {
		if ( idx != 0 ) catCrlf(); //delimiter

		auto row = fmt::MemoryWriter {};
		row << fmt::pad(fmt::hexu(idx), 4, '0');
		auto i = size_t { 0 };
		while ( i < stc_bytesPerLine && idx < size ) {
			row << ' ' << fmt::pad(fmt::hexu(mem[idx]), 2, '0');
			i ++; idx ++;
		}
		cat(row.c_str());
	}
}

void CStrWriter::catSize(std::size_t size) {
	cat(strf("%d", size));
}

void CStrWriter::catPtr(void const* ptr) {
	cat(strf("%p", ptr));
}

void CStrWriter::catDump(void const* data, size_t bufsize)
{
	assert(bufsize == 0 || data);

	static auto const stc_maxsize = size_t { 0x10000 };
	auto size = bufsize;

	if ( size > stc_maxsize ) {
		catln(strf("全%d[byte]の内、%d[byte]のみダンプします。", bufsize, stc_maxsize));
		size = stc_maxsize;
	}

	catln("dump  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
	catln("----------------------------------------------------");
	catDumpImpl(data, size);
}
