//! テストフレームワーク

#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <functional>
#include <vector>

class TestClass;
class TestCaseContext;
class TestFramework;
class TestSuite;
class TestSuiteContext;

using TestBodyFun = std::function<bool(TestCaseContext& t)>;

class TestCase {
	std::string title_;
	TestBodyFun body_;

public:
	TestCase(std::string&& title, TestBodyFun&& body)
		: title_(std::move(title))
		, body_(std::move(body))
	{
	}

	auto title() const -> std::string_view {
		return title_;
	}

	auto title_contains(std::string_view const& filter) const -> bool {
		return title().find(filter) != std::string::npos;
	}

	bool run(TestCaseContext& context) const {
		return body_(context);
	}
};

class TestSuite {
	std::string title_;
	std::vector<TestCase> cases_;

public:
	TestSuite(std::string&& title)
		: title_(std::move(title))
		, cases_()
	{
	}

	auto title() const -> std::string_view {
		return title_;
	}

	auto title_contains(std::string_view const& filter) const -> bool {
		return title().find(filter) != std::string::npos;
	}

	auto add_case(TestCase&& test_case) {
		cases_.emplace_back(std::move(test_case));
	}

	void run(TestFramework& framework);
};

class TestSuiteContext {
	std::size_t suite_id_;
	TestFramework& framework_;

public:
	TestSuiteContext(std::size_t suite_id, TestFramework& framework)
		: suite_id_(suite_id)
		, framework_(framework)
	{
	}

	void test(char const* title, std::function<bool(TestCaseContext&)> body);
};

class TestFramework {
	std::size_t pass_count_;
	std::size_t fail_count_;
	std::size_t skip_count_;

	std::string filter_;
	std::vector<TestSuite> suites_;

public:
	TestFramework()
		: pass_count_()
		, fail_count_()
		, skip_count_()
		, suites_()
	{
	}

	void only(std::string&& filter) {
		filter_ = std::move(filter);
	}

	auto new_suite(char const* title) -> TestSuiteContext {
		auto suite_id = suites_.size();
		suites_.emplace_back(std::string{ title });
		return TestSuiteContext{ suite_id, *this };
	}

	auto add_case(std::size_t suite_id, TestCase&& test_case) {
		suites_.at(suite_id).add_case(std::move(test_case));
	}

	bool may_run(TestSuite const& suite, TestCase const& test_case) const {
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

	bool run();

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

class TestCaseContext {
	std::string title_;
	std::size_t assert_count_;

	TestSuite const& suite_;
	TestFramework& framework_;

public:
	explicit TestCaseContext(std::string&& title, TestSuite const& suite, TestFramework& framework)
		: title_(std::move(title))
		, assert_count_()
		, suite_(suite)
		, framework_(framework)
	{
	}

	template<typename T, typename U>
	bool eq(T&& actual, U&& expected) {
		assert_count_++;

		if (actual != expected) {
			std::cerr << u8"  実際の値が期待される値と異なります:" << std::endl;
			std::cerr << u8"    ✔ 期待される値: " << expected << std::endl;
			std::cerr << u8"    ✘ 実際の値: " << actual << std::endl;
			assert(false && u8"表明違反");
			return false;
		}

		return true;
	}

	bool finish() const {
		if (assert_count_ == 0) {
			std::cerr << u8"    表明が実行されなかったため、テストは失敗とみなされます。" << std::endl;
			return false;
		}

		return true;
	}
};
