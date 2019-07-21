
#include "pch.h"
#include <cassert>
#include <cstring>
#include "../string_split.h"
#include "strf.h"
#include "CStrWriter.h"
#include "CStrBuf.h"

auto as_view(CStrWriter const& writer) -> Utf8StringView {
	return as_utf8(writer.get().data());
}

CStrWriter::CStrWriter()
	: buf_(std::make_shared<CStrBuf>())
{
	buf_->limit(0x8000);
}

auto CStrWriter::get() const -> std::string const& { return buf_->get(); }

auto CStrWriter::is_full() const -> bool {
	return buf_->is_full();
}

void CStrWriter::indent() {
	depth_++;
}

void CStrWriter::unindent() {
	if (depth_ == 0) {
		assert(false && u8"indent と unindent が対応していません。");
		return;
	}

	depth_--;
}

void CStrWriter::set_limit(std::size_t limit) {
	buf_->limit(limit);
}

// バッファの末尾に文字列を足す。
// 行ごとに分割して適切に字下げを挿入する。
void CStrWriter::cat(char const* s)
{
	auto first = true;

	for (auto&& line : StringLines{ std::string_view{ s } }) {
		if (!first) {
			buf_->append("\r\n");
			head_ = true;
		}
		first = false;

		if (line.empty()) {
			continue;
		}

		if (head_) {
			for (auto i = std::size_t{}; i < depth_; i++) {
				// FIXME: 字下げをスペースで行う？
				buf_->append("\t");
			}
			head_ = false;
		}
		buf_->append(line.data(), line.size());
	}
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
		catln(strf(u8"全%d[byte]の内、%d[byte]のみダンプします。", bufsize, stc_maxsize));
		size = stc_maxsize;
	}

	catln("dump  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
	catln("----------------------------------------------------");
	catDumpImpl(data, size);
}
