//! テストフレームワーク

#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <functional>
#include <vector>

class TestCase;
class TestCaseContext;
class TestFramework;
class TestSuite;
class TestSuiteContext;

// テストケースの本体となる、表明を行うために実行される関数の型。
// ラムダ式から暗黙に変換できる。
using TestBodyFun = std::function<bool(TestCaseContext& t)>;

// テストケース。表明を行うために実行される関数に名前をつけたもの。
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

// テストスイート。テストケースをグループ化するも。
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

// テストスイートを構築するもの
class TestSuiteContext {
	std::size_t suite_id_;
	TestFramework& framework_;

public:
	TestSuiteContext(std::size_t suite_id, TestFramework& framework)
		: suite_id_(suite_id)
		, framework_(framework)
	{
	}

	// テストケースを宣言する。
	// 使い方: `suite.test(u8"タイトル", [&](TestCaseContext& t) { ..; return true; })`
	void test(char const* title, std::function<bool(TestCaseContext&)> body);
};

// テストフレームワーク。テストを実行するもの。
class TestFramework {
	std::size_t pass_count_;

	std::size_t fail_count_;

	std::size_t skip_count_;

	// 実行するテストを絞り込むための文字列
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

	// 指定された名前を含むテストスイートやテストケースだけ実行するように設定する。
	void only(std::string&& filter) {
		filter_ = std::move(filter);
	}

	// 新しいテストスイートを生成する。
	// 使い方: `auto suite = framework.new_suite(u8"タイトル"); suite.test(..); ..`
	auto new_suite(char const* title) -> TestSuiteContext {
		auto suite_id = suites_.size();
		suites_.emplace_back(std::string{ title });
		return TestSuiteContext{ suite_id, *this };
	}

	// 宣言されたテストケースを記録する。(まだ実行しない。)
	auto add_case(std::size_t suite_id, TestCase&& test_case) {
		suites_.at(suite_id).add_case(std::move(test_case));
	}

	// テストケースを実行するか？
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

	// 2つの値が等しいことを表明する。
	// 注意: 2つの値が `operator ==` で比較可能で、
	// `operator <<` でストリームに出力できるものでなければコンパイルエラーになる。
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

	// テストケースの実行が完了したときに、フレームワークから呼ばれる。
	bool finish() const {
		if (assert_count_ == 0) {
			std::cerr << u8"    表明が実行されなかったため、テストは失敗とみなされます。" << std::endl;
			return false;
		}

		return true;
	}
};
