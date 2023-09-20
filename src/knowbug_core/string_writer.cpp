
#include "pch.h"
#include <cassert>
#include <cstring>
#include "string_split.h"
#include "string_writer.h"

static auto const TRIMMED_SUFFIX = as_utf8(u8"(too long)");

static auto const DEFAULT_LIMIT = std::size_t{ 0x2000 };

StringWriter::StringWriter()
	: buf_()
	, depth_()
	, head_(true)
	, limit_()
{
	set_limit(DEFAULT_LIMIT);
}

auto StringWriter::is_full() const -> bool {
	return limit_ == 0;
}

auto StringWriter::as_view() const -> std::u8string_view {
	return buf_;
}

auto StringWriter::finish() -> std::u8string&& {
	return std::move(buf_);
}

void StringWriter::indent() {
	depth_++;
}

void StringWriter::unindent() {
	if (depth_ == 0) {
		assert(false && u8"indent と unindent が対応していません。");
		return;
	}

	depth_--;
}

void StringWriter::set_limit(std::size_t limit) {
	limit_ = limit;

	buf_.reserve(buf_.size() + limit_);
}

// バッファの末尾に文字列を追加する。
// 文字列制限が上限に達したら打ち切る。
void StringWriter::cat_limited(std::u8string_view str) {
	if (is_full()) {
		return;
	}

	if (str.size() + TRIMMED_SUFFIX.size() > limit_) {
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
void StringWriter::cat_by_lines(std::u8string_view str) {
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
				cat_limited(u8"  ");
			}
			head_ = false;
		}
		cat_limited(line);
	}
}

// メモリダンプ文字列を書き込む。
// 最後の行は改行を挿入しない。
void StringWriter::cat_memory_dump_impl(void const* data, std::size_t size) {
	static auto const BYTE_COUNT_PER_LINE = std::size_t{ 0x10 };

	auto mem = static_cast<unsigned char const*>(data);
	auto idx = std::size_t{};
	auto row = std::string{};
	while (idx < size) {
		if (idx != 0) {
			cat_crlf();
		}

		row.clear();

		// インデックス
		{
			char temp[8] = "";
			sprintf_s(temp, "%04X", (unsigned int)idx);
			row += temp;
		}

		auto i = std::size_t{};
		while (i < BYTE_COUNT_PER_LINE && idx < size) {
			row += ' ';
			// バイト
			{
				char temp[8] = "";
				sprintf_s(temp, "%02X", (unsigned int)mem[idx]);
				row += temp;
			}
			i++; idx++;
		}
		cat(as_utf8(row));
	}
}

void StringWriter::cat_size(std::size_t size) {
	char temp[16] = "";
	sprintf_s(temp, "%lld", (long long int)size);
	cat(ascii_as_utf8(temp));
}

// ポインタのアドレスを書き込む
// (小文字16進数8桁, e.g. `0x0123cdef`)
void StringWriter::cat_ptr(void const* ptr) {
	char temp[16] = "";
	auto len = sprintf_s(temp, "%p", ptr);
	for (auto i = 0; i < len; i++) {
		temp[i] = std::tolower(temp[i]);
	}

	cat(u8"0x");
	cat(ascii_as_utf8(temp));
}

void StringWriter::cat_memory_dump(void const* data, std::size_t data_size) {
	assert(data_size == 0 || data != nullptr);

	// 1バイトあたり約3バイトの文字列が生成されるので、残り文字数 / 3 で打ち切る。
	auto max_size = std::min(data_size, std::min(limit_ / 3, std::size_t{ 0x10000 }));

	auto dump_size = data_size;
	if (dump_size > max_size) {
		cat(u8"全");
		cat_size(data_size);
		cat(u8"[byte]の内、");
		cat_size(max_size);
		cat_line(u8"[byte]のみダンプします。");
		dump_size = max_size;
	}

	cat_line("dump  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
	cat_line("----------------------------------------------------");
	cat_memory_dump_impl(data, dump_size);
}

// -----------------------------------------------
// Tests
// -----------------------------------------------

static auto string_writer_new() -> StringWriter {
	return StringWriter{};
}

void string_writer_tests(Tests& tests) {
	auto& suite = tests.suite(u8"str_writer_tests");

	suite.test(
		u8"上限を超えると打ち切られる",
		[&](TestCaseContext& t) {
			auto w = string_writer_new();
			w.set_limit(20);

			w.cat(u8"0123456789<長すぎる部分は省略されます>");
			if (!t.eq(as_view(w), u8"0123456789(too long)")) {
				return false;
			}

			// これ以上の追記は無意味。
			w.cat(u8"add");
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
			w.cat_line(u8"親");
			w.indent();

			w.cat_line(u8"兄");
			w.indent();
			w.cat_line(u8"甥");
			w.unindent();

			w.cat_line(u8"本人");
			w.indent();
			// 途中に改行があっても字下げされる。(LF は CRLF に置き換わる。)
			w.cat_line(u8"長男\n長女");

			auto expected = as_utf8(
				u8"親\r\n"
				u8"  兄\r\n"
				u8"    甥\r\n"
				u8"  本人\r\n"
				u8"    長男\r\n"
				u8"    長女\r\n"
			);
			return t.eq(as_view(w), expected);
		});

	suite.test(
		u8"ポインタを文字列化できる",
		[&](TestCaseContext& t) {
			{
				auto w = string_writer_new();
				auto dead_beef = (void const*)(UINT_PTR)0xdeadbeef;
				w.cat_ptr(dead_beef);
				if (!t.eq(as_view(w), u8"0xdeadbeef")) {
					return false;
				}
			}

			{
				auto w = string_writer_new();
				w.cat_ptr(nullptr);
				// <nullptr> とか (nil) とかでも可
				if (!t.eq(as_view(w), u8"0x00000000")) {
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

			auto buf = std::vector<char8_t>{};
			buf.resize(size, u8'\0');
			std::copy(t1.begin(), t1.end(), &buf[0]);
			std::copy(t2.begin(), t2.end(), &buf[t1.size() + 1]);

			w.cat_memory_dump(buf.data(), buf.size());
			w.cat_crlf();

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
