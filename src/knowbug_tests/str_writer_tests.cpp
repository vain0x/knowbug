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
};
