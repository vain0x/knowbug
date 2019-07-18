#include <memory>
#include "../module/CStrBuf.h"
#include "../module/CStrWriter.h"
#include "../encoding.h"
#include "str_writer_tests.h"

static auto str_writer_new(std::size_t limit = 0x8000) -> CStrWriter {
	auto buf = std::make_shared<CStrBuf>();
	buf->limit(limit);

	return CStrWriter{ std::move(buf) };
}

void str_writer_tests(TestFramework& framework) {
	auto suite = framework.new_suite(u8"str_writer_tests");

	suite.test(
		u8"上限を超えると打ち切られる",
		[&](TestCaseContext& t) {
			auto w = str_writer_new(20);

			w.cat(as_utf8(u8"0123456789<trimmed>"));
			if (!t.eq(as_view(w), as_utf8(u8"0123456789(too long)"))) {
				return false;
			}

			// これ以上の追記は無意味。
			w.cat(as_utf8(u8"add"));
			if (!t.eq(w.get().length(), 20)) {
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
			auto w = str_writer_new();
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
				auto w = str_writer_new();
				auto dead_beef = (void const*)0xdeadbeef;
				w.catPtr(dead_beef);
				if (!t.eq(as_view(w), as_utf8(u8"0xdeadbeef"))) {
					return false;
				}
			}

			{
				auto w = str_writer_new();
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
			auto w = str_writer_new();

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
