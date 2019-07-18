//! テストを実行するもの

#pragma once

#include <cassert>
#include <sstream>
#include "../test_suite.h"

class TestRunner;
class TestCaseRunnerImpl;

class TestRunner {
	std::size_t pass_count_;

	std::size_t fail_count_;

	std::size_t skip_count_;

	// 実行するテストを絞り込むための文字列
	std::string filter_;

	Tests tests_;

public:
	TestRunner()
		: pass_count_()
		, fail_count_()
		, skip_count_()
		, filter_()
		, tests_()
	{
	}

	auto tests() -> Tests& {
		return tests_;
	}

	// 指定された名前を含むテストスイートやテストケースだけ実行するように設定する。
	void only(std::string&& filter) {
		filter_ = std::move(filter);
	}

	// テストケースを実行するか？
	auto may_run(TestSuite const& suite, TestCase const& test_case) const -> bool {
		return suite.title_contains(filter_) || test_case.title_contains(filter_);
	}

	void did_pass() {
		pass_count_++;
	}

	void did_fail() {
		fail_count_++;
	}

	void did_skip() {
		skip_count_++;
	}

	auto run() -> bool;

private:
	bool is_successful() const {
		return pass_count_ > 0 && fail_count_ == 0 && skip_count_ == 0;
	}

	bool is_failed() const {
		return fail_count_ > 0;
	}

	auto test_count() const -> std::size_t {
		return pass_count_ + fail_count_ + skip_count_;
	}
};

class TestCaseRunner
	: public TestCaseContext
{
	TestCase const& case_;

	std::size_t assert_count_;

	std::ostream& output_;

public:
	TestCaseRunner(TestCase const& test_case, std::ostream& output)
		: case_(test_case)
		, assert_count_()
		, output_(output)
	{
	}

	auto output()->std::ostream & override {
		return output_;
	}

	void did_pass() override {
		assert_count_++;
	}

	void did_mismatch(WriteFun write_actual, WriteFun write_expected) override {
		assert_count_++;

		output() << u8"  実際の値が期待される値と異なります:" << std::endl;
		output() << u8"    ✔ 期待される値: ";
		write_expected(output());
		output() << std::endl;

		std::cout << u8"    ✘ 実際の値: ";
		write_actual(output());
		output() << std::endl;

		assert(false && u8"表明違反");
	}

	auto finish() -> bool {
		if (assert_count_ == 0) {
			output() << u8"    表明が実行されなかったため、テストは失敗とみなされます。" << std::endl;
			return false;
		}

		return true;
	}
};
