//! テストフレームワーク

#include "knowbug_tests_framework.h"

void TestSuite::run(TestFramework& framework) {
	std::cerr << u8"Running " << title() << u8".." << std::endl;

	for (auto&& test_case : cases_) {
		if (!framework.may_run(*this, test_case)) {
			framework.did_skip();
			continue;
		}

		auto context = TestCaseContext{ std::string{ test_case.title() }, *this, framework };

		std::cerr << u8"  Running " << test_case.title() << u8".." << std::endl;
		auto pass = test_case.run(context) && context.finish();

		if (pass) {
			framework.did_pass();
		} else {
			std::cerr << u8"    Test failed." << std::endl;
			framework.did_fail();
		}
	}
}

TestSuiteContext::~TestSuiteContext() {
	framework_.add_suite(TestSuite{ std::move(title_), std::move(cases_) });
}

bool TestFramework::run() {
	for (auto&& suite : suites_) {
		suite.run(*this);
	}

	if (!is_successful()) {
		std::cerr << std::endl;
		std::cerr << u8"NOT all of tests passed:" << std::endl;
		std::cerr
			<< u8"  "
			<< pass_count_ << u8" passed / "
			<< fail_count_ << u8" failed / "
			<< skip_count_ << u8" skipped / "
			<< test_count() << u8" all"
			<< std::endl;
		return false;
	}

	std::cerr << u8"All " << test_count() << u8" tests passed. Conguratulations!" << std::endl;
	return true;
}
