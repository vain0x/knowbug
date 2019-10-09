
#include "pch.h"
#include <cassert>
#include <cstring>
#include "string_split.h"
#include "string_writer.h"

static auto const TRIMMED_SUFFIX = std::string_view{ u8"(too long)" };

static auto const DEFAULT_LIMIT = 0x8000;

CStrWriter::CStrWriter()
	: buf_()
	, depth_()
	, head_(true)
	, limit_()
{
	set_limit(DEFAULT_LIMIT);
}

auto CStrWriter::is_full() const -> bool {
	return limit_ == 0;
}

auto CStrWriter::as_view() const -> Utf8StringView {
	return as_utf8(buf_);
}

auto CStrWriter::finish() -> std::string&& {
	return std::move(buf_);
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
	limit_ = limit;

	buf_.reserve(buf_.size() + limit_);
}

// バッファの末尾に文字列を追加する。
// 文字列制限が上限に達したら打ち切る。
void CStrWriter::cat_limited(std::string_view const& str) {
	if (is_full()) {
		return;
	}

	if (str.size() > limit_) {
		if (limit_ < TRIMMED_SUFFIX.size()) {
			buf_ += str.substr(0, limit_);
			limit_ = 0;
			return;
		}

		buf_ += str.substr(0, limit_ - TRIMMED_SUFFIX.size());
		buf_ += TRIMMED_SUFFIX;
		limit_ = 0;
		return;
	}

	buf_ += str;
	limit_ -= str.size();
}

// バッファの末尾に文字列を追加する。
// 行ごとに分割して適切に字下げを挿入する。
void CStrWriter::cat_by_lines(std::string_view const& str) {
	auto first = true;

	for (auto&& line : StringLines{ str }) {
		if (is_full()) {
			return;
		}

		if (!first) {
			cat_limited(u8"\r\n");
			head_ = true;
		}
		first = false;

		if (line.empty()) {
			continue;
		}

		if (head_) {
			for (auto i = std::size_t{}; i < depth_; i++) {
				// FIXME: 字下げをスペースで行う？
				cat_limited(u8"\t");
			}
			head_ = false;
		}
		cat_limited(line);
	}
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
