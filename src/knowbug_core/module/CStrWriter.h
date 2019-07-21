#pragma once

#include <cassert>
#include <memory>
#include <string>
#include "../encoding.h"
#include "strf.h"

// 文字列を構築するためのもの。
// 自動的な字下げと文字数制限の機能を持つ。
class CStrWriter {
	std::string buf_;

	// 字下げの深さ
	std::size_t depth_;

	// 次の文字が行頭になるかどうか
	bool head_;

	// 書き込み可能な残りの文字数
	std::size_t limit_;

public:
	CStrWriter();

public:
	auto is_full() const -> bool;

	auto as_view() const->Utf8StringView;

	void cat(std::string_view const& str) {
		cat_by_lines(str);
	}

	void catln(std::string_view const& str) {
		cat(str);
		catCrlf();
	}

	void cat(Utf8StringView const& str) {
		cat(as_native(str));
	}

	void catln(Utf8StringView const& str) {
		catln(as_native(str));
	}

	void catCrlf() {
		cat(u8"\r\n");
	}

	void catSize(std::size_t size);

	void catPtr(void const* ptr);

	void catDump(void const* data, size_t size);

	// 字下げを1段階深くする。
	void indent();

	// 字下げを1段階浅くする。
	void unindent();

	// 残りの書き込み文字数の制限を変更する。
	// すでに書き込まれた文字列には影響しない。
	void set_limit(std::size_t limit);

	auto finish() -> std::string&&;

private:
	void cat_by_lines(std::string_view const& str);

	void cat_limited(std::string_view const& str);

	void catDumpImpl(void const* data, std::size_t size);
};

inline static auto as_view(CStrWriter const& writer) -> Utf8StringView {
	return writer.as_view();
}
