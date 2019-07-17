#include <iostream>
#include "../encoding.h"
#include "knowbug_tests_framework.h"

static void enable_utf_8() {
	SetConsoleOutputCP(CP_UTF8);
	std::setvbuf(stdout, nullptr, _IOFBF, 1024);
}

static void hello_tests(TestFramework& framework) {
	auto suite = framework.new_suite(u8"hello");

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
		u8"pass",
		[&](TestCaseContext& t) {
			t.eq(0, 0);
			return true;
		});
}

auto main() -> int {
	enable_utf_8();
	auto framework = TestFramework{};

	// HINT: ここで framework.only("foo") とすると foo という名前を含むテストだけ実行されます。

	// ここにテストスイートを列挙します。
	hello_tests(framework);

	auto success = framework.run();
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
