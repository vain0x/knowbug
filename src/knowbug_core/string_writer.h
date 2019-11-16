#pragma once

#include <cassert>
#include <memory>
#include <string>
#include "encoding.h"
#include "string_format.h"
#include "test_suite.h"

// 文字列を構築するためのもの。
// 自動的な字下げと文字数制限の機能を持つ。
class StringWriter {
	Utf8String buf_;

	// 字下げの深さ
	std::size_t depth_;

	// 次の文字が行頭になるかどうか
	bool head_;

	// 書き込み可能な残りの文字数
	std::size_t limit_;

public:
	StringWriter();

public:
	auto is_full() const -> bool;

	auto indent_length() const -> std::size_t {
		return depth_ * 2;
	}

	auto as_view() const->Utf8StringView;

	void cat(std::string_view const& str) {
		cat(as_utf8(str));
	}

	void cat_line(std::string_view const& str) {
		cat_line(as_utf8(str));
	}

	void cat(Utf8StringView str) {
		cat_by_lines(str);
	}

	void cat_line(Utf8StringView str) {
		cat(str);
		cat_crlf();
	}

	void cat_crlf() {
		cat(u8"\r\n");
	}

	void cat_size(std::size_t size);

	void cat_ptr(void const* ptr);

	void cat_memory_dump(void const* data, std::size_t size);

	// 字下げを1段階深くする。
	void indent();

	// 字下げを1段階浅くする。
	void unindent();

	// 残りの書き込み文字数の制限を変更する。
	// すでに書き込まれた文字列には影響しない。
	void set_limit(std::size_t limit);

	auto finish() -> Utf8String&&;

private:
	void cat_by_lines(Utf8StringView str);

	void cat_limited(Utf8StringView str);

	void cat_memory_dump_impl(void const* data, std::size_t size);
};

inline static auto as_view(StringWriter const& writer) -> Utf8StringView {
	return writer.as_view();
}

void string_writer_tests(Tests& tests);
