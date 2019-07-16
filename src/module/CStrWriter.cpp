
#include <cassert>
#include <cstring>
#include "strf.h"

#include "CStrWriter.h"
#include "CStrBuf.h"

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

// バッファの末尾に文字列を足す。
// 行ごとに分割して適切に字下げを挿入する。
void CStrWriter::cat(char const* s)
{
	// 現在の行頭の位置
	auto l = std::size_t{};

	// 現在の位置 (l..r の間には改行がない)
	auto r = l;

	while (true) {
		if (s[r] == '\0') {
			buf_->append(s + l, r - l);
			break;
		}

		if (s[r] == '\r' || s[r] == '\n') {
			// いま見ている行と改行を挿入する。改行は常に CRLF を使う。
			buf_->append(s + l, r - l);
			buf_->append("\r\n");

			if (s[r] == '\r' && s[r + 1] == '\n') {
				r += 2;
			} else {
				r++;
			}

			l = r;
			head_ = true;
			continue;
		}

		// 行の最初の文字の前に字下げを挿入する。
		if (head_) {
			for (auto i = std::size_t{}; i < depth_; i++) {
				// FIXME: 字下げをスペースで行う？
				buf_->append("\t");
			}
			head_ = false;
		}

		r++;
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
