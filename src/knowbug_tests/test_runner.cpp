//! テストフレームワーク

#include "test_runner.h"

static void run_suite(TestSuite const& suite, TestRunner& runner) {
	std::cout << u8"テストスイート " << suite.title() << u8".." << std::endl;

	for (auto&& test_case : suite.cases()) {
		if (!runner.may_run(suite, test_case)) {
			runner.did_skip();
			continue;
		}

		auto case_context = TestCaseRunner{ test_case, std::cout };

		case_context.output() << u8"  テスト " << test_case.title() << u8".." << std::endl;

		auto success = test_case.body()(case_context) && case_context.finish();
		if (success) {
			runner.did_pass();
		} else {
			case_context.output() << u8"    失敗" << std::endl;
			runner.did_fail();
		}
	}
}

auto TestRunner::run() -> bool {
	for (auto&& suite : tests_.suites()) {
		run_suite(suite, *this);
	}

	if (!is_successful()) {
		std::cout << std::endl;
		std::cout << u8"結果:" << std::endl;
		std::cout
			<< u8"  "
			<< u8"成功 " << pass_count_ << u8" 件 / "
			<< u8"失敗 " << fail_count_ << u8" 件 / "
			<< u8"スキップ " << skip_count_ << u8" 件 / "
			<< u8"合計 " << test_count() << u8" 件"
			<< std::endl;
		return false;
	}

	std::cout << u8"全 " << test_count() << u8" 件のテストがすべて成功しました。Congratulations!" << std::endl;
	return true;
}
