//! テストフレームワーク

#include "knowbug_tests_framework.h"

void TestSuite::run(TestFramework& framework) {
	std::cout << u8"テストスイート " << title() << u8".." << std::endl;

	for (auto&& test_case : cases_) {
		if (!framework.may_run(*this, test_case)) {
			framework.did_skip();
			continue;
		}

		auto context = TestCaseContext{ std::string{ test_case.title() }, *this, framework };

		std::cout << u8"  テスト " << test_case.title() << u8".." << std::endl;
		auto pass = test_case.run(context) && context.finish();

		if (pass) {
			framework.did_pass();
		} else {
			std::cout << u8"    失敗" << std::endl;
			framework.did_fail();
		}
	}
}

void TestSuiteContext::test(char const* title, std::function<bool(TestCaseContext&)> body) {
	framework_.add_case(suite_id_, TestCase{ std::string{ title }, std::move(body) });
}

bool TestFramework::run() {
	for (auto&& suite : suites_) {
		suite.run(*this);
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
