//! テストケースを宣言する仕組み

#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

class TestCase;
class TestCaseContext;
class Tests;
class TestSuite;

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

	auto body() const -> TestBodyFun const& {
		return body_;
	}
};

// テストスイート。テストケースをグループ化するもの。
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

	auto cases() const -> std::vector<TestCase> const& {
		return cases_;
	}

	// テストケースを宣言する。
	// 使い方: `suite.test(u8"タイトル", [&](TestCaseContext& t) { ..; return true; })`
	void test(char const* title, TestBodyFun body) {
		cases_.emplace_back(std::string{ title }, std::move(body));
	}
};

// テストケースの集まり。
// テストケースはテストスイートでグループ化されている。
// 使い方: `auto& suite = tests.suite(..)`
class Tests {
	std::vector<TestSuite> suites_;

public:
	Tests()
		: suites_()
	{
	}

	auto suites() const -> std::vector<TestSuite> const& {
		return suites_;
	}

	// テストスイートを宣言する。
	// 使い方: `auto& suite = tests.suite(u8"タイトル"); suite.test(..); ..`
	auto suite(char const* title) -> TestSuite& {
		return suites_.emplace_back(std::string{ title });
	}
};

// 実行中のテストケースからテストランナーへのインターフェイス。
class TestCaseContext {
public:
	using WriteFun = std::function<void(std::ostream&)>;

	TestCaseContext() {
	}

	virtual ~TestCaseContext() {
	}

	virtual auto output() -> std::ostream& = 0;

	// 表明に成功したときに呼ばれる。
	virtual void did_pass() = 0;

	// 比較の表明に失敗したときに呼ばれる。
	virtual void did_mismatch(WriteFun write_actual, WriteFun write_expected) = 0;

	// 2つの値が等しいことを表明する。
	// 注意: 2つの値が `operator ==` で比較可能で、
	// `operator <<` でストリームに出力できるものでなければコンパイルエラーになる。
	template<typename T, typename U>
	auto eq(T&& actual, U&& expected) -> bool {
		if (actual != expected) {
			did_mismatch(
				[&](std::ostream& out) {
					out << actual;
				},
				[&](std::ostream& out) {
					out << expected;
				});
			return false;
		}

		did_pass();
		return true;
	}

	// コピー・ムーブともに禁止。
	TestCaseContext(TestCaseContext const& other) = delete;

	TestCaseContext(TestCaseContext&& other) = delete;

	auto operator =(TestCaseContext const& other)->TestCaseContext & = delete;

	auto operator =(TestCaseContext&& other)->TestCaseContext & = delete;
};
