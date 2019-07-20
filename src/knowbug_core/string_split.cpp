#include "pch.h"
#include "encoding.h"
#include "string_split.h"

void string_to_lines_tests(Tests& tests) {
	auto& suite = tests.suite(u8"string_to_lines");

	suite.test(
		u8"改行ごとに分割する",
		[&](TestCaseContext& t) {
			auto source =
				u8"いろはにほへと\r\n"
				u8"ちりぬるを\n";
			auto lines = string_to_lines(std::string{ source });

			if (!t.eq(lines.size(), 3)) {
				return false;
			}

			if (!t.eq(lines.at(0), u8"いろはにほへと")) {
				return false;
			}

			if (!t.eq(lines.at(1), u8"ちりぬるを")) {
				return false;
			}

			if (!t.eq(lines.at(2).size(), 0)) {
				return false;
			}

			return true;
		});

	suite.test(
		u8"改行で終わらない文字列を分割する",
		[&](TestCaseContext& t) {
			auto lines = string_to_lines(to_owned(as_utf8(u8"いろはにほへと")));
			return t.eq(lines.size(), 1)
				&& t.eq(lines.at(0), as_utf8("いろはにほへと"));
		});

	suite.test(
		u8"空文字列を分割する",
		[&](TestCaseContext& t) {
			auto lines = string_to_lines(Utf8String{});
			return t.eq(lines.size(), 1)
				&& t.eq(lines.at(0).size(), 0);
		});
}
