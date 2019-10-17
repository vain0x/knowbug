
#include "pch.h"
#include <cassert>
#include <cstring>
#include "string_split.h"
#include "string_writer.h"

static auto const TRIMMED_SUFFIX = as_utf8(u8"(too long)");

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
	return buf_;
}

auto CStrWriter::finish() -> Utf8String&& {
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
void CStrWriter::cat_limited(Utf8StringView str) {
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
void CStrWriter::cat_by_lines(Utf8StringView str) {
	auto first = true;

	for (auto&& line : StringLines{ str }) {
		if (is_full()) {
			return;
		}

		if (!first) {
			cat_limited(as_utf8(u8"\r\n"));
			head_ = true;
		}
		first = false;

		if (line.empty()) {
			continue;
		}

		if (head_) {
			for (auto i = std::size_t{}; i < depth_; i++) {
				// FIXME: 字下げをスペースで行う？
				cat_limited(as_utf8(u8"\t"));
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

// -----------------------------------------------
// Tests
// -----------------------------------------------

static auto string_writer_new() -> CStrWriter {
	return CStrWriter{};
}

void string_writer_tests(Tests& tests) {
	auto& suite = tests.suite(u8"str_writer_tests");

	suite.test(
		u8"上限を超えると打ち切られる",
		[&](TestCaseContext& t) {
			auto w = string_writer_new();
			w.set_limit(20);

			w.cat(as_utf8(u8"0123456789<長すぎる部分は省略されます>"));
			if (!t.eq(as_view(w), as_utf8(u8"0123456789(too long)"))) {
				return false;
			}

			// これ以上の追記は無意味。
			w.cat(as_utf8(u8"add"));
			if (!t.eq(as_view(w).size(), 20)) {
				return false;
			}

			// 追記ができないことが判定できる。
			if (!t.eq(w.is_full(), true)) {
				return false;
			}

			return true;
		});

	suite.test(
		u8"適切に字下げできる",
		[&](TestCaseContext& t) {
			auto w = string_writer_new();
			w.catln(as_utf8(u8"親"));
			w.indent();

			w.catln(as_utf8(u8"兄"));
			w.indent();
			w.catln(as_utf8(u8"甥"));
			w.unindent();

			w.catln(as_utf8(u8"本人"));
			w.indent();
			// 途中に改行があっても字下げされる。(LF は CRLF に置き換わる。)
			w.catln(as_utf8(u8"長男\n長女"));

			auto expected = as_utf8(
				u8"親\r\n"
				u8"\t兄\r\n"
				u8"\t\t甥\r\n"
				u8"\t本人\r\n"
				u8"\t\t長男\r\n"
				u8"\t\t長女\r\n"
			);
			return t.eq(as_view(w), expected);
		});

	suite.test(
		u8"ポインタを文字列化できる",
		[&](TestCaseContext& t) {
			{
				auto w = string_writer_new();
				auto dead_beef = (void const*)0xdeadbeef;
				w.catPtr(dead_beef);
				if (!t.eq(as_view(w), as_utf8(u8"0xdeadbeef"))) {
					return false;
				}
			}

			{
				auto w = string_writer_new();
				w.catPtr(nullptr);
				// <nullptr> とか 0x0000 とかでも可
				if (!t.eq(as_view(w), as_utf8(u8"(nil)"))) {
					return false;
				}
			}

			return true;
		});

	suite.test(
		u8"メモリダンプを出力できる",
		[&](TestCaseContext& t) {
			auto w = string_writer_new();

			auto t1 = as_utf8(u8"いろはにほへとちりぬるを");
			auto t2 = as_utf8(u8"わかよたれそつねならむ");
			auto size = t1.size() + 1 + t2.size() + 2;

			auto buf = std::vector<Utf8Char>{};
			buf.resize(size, Utf8Char{});
			std::copy(t1.begin(), t1.end(), &buf[0]);
			std::copy(t2.begin(), t2.end(), &buf[t1.size() + 1]);

			w.catDump(buf.data(), buf.size());
			w.catCrlf();

			auto expected = as_utf8(
				u8"dump  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n"
				u8"----------------------------------------------------\r\n"
				u8"0000 E3 81 84 E3 82 8D E3 81 AF E3 81 AB E3 81 BB E3\r\n"
				u8"0010 81 B8 E3 81 A8 E3 81 A1 E3 82 8A E3 81 AC E3 82\r\n"
				u8"0020 8B E3 82 92 00 E3 82 8F E3 81 8B E3 82 88 E3 81\r\n"
				u8"0030 9F E3 82 8C E3 81 9D E3 81 A4 E3 81 AD E3 81 AA\r\n"
				u8"0040 E3 82 89 E3 82 80 00 00\r\n"
			);
			return t.eq(as_view(w), expected);
		});
};
