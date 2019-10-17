//! テストアプリのエントリーポイント

#include "pch.h"
#include <iostream>
#include "../knowbug_core/hsp_objects_module_tree.h"
#include "../knowbug_core/hsp_object_writer.h"
#include "../knowbug_core/source_files.h"
#include "../knowbug_core/string_split.h"
#include "../knowbug_core/string_writer.h"
#include "test_runner.h"

static void enable_utf_8() {
	SetConsoleOutputCP(CP_UTF8);
	std::setvbuf(stdout, nullptr, _IOFBF, 1024);
}

// テストの書き方のサンプル
static void hello_tests(Tests& tests) {
	auto& suite = tests.suite(u8"hello");

	suite.test(
		u8"add",
		[&](TestCaseContext& t) {
			if (!t.eq(2 + 3, 5)) {
				return false;
			}

			if (!t.eq(3 + 7, 10)) {
				return false;
			}

			return true;
		});

	suite.test(
		u8"ok",
		[&](TestCaseContext& t) {
			return t.eq(0, 0);
		});
}

auto main() -> int {
	enable_utf_8();
	auto runner = TestRunner{};
	auto& tests = runner.tests();

	// HINT: ここで framework.only("foo") とすると foo という名前を含むテストだけ実行される。

	// ここにテストスイートを列挙する。
	hello_tests(tests);
	string_writer_tests(tests);
	module_tree_tests(tests);
	hsp_object_writer_tests(tests);
	source_files_tests(tests);
	string_lines_tests(tests);

	auto success = runner.run();
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
