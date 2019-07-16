// 文字列への整形書き込み

#ifndef IG_CLASS_STRING_WRITER_H
#define IG_CLASS_STRING_WRITER_H

#include <string>
#include <stack>
#include <memory>
#include <cassert>
#include "../encoding.h"
#include "strf.h"

class CStrBuf;

//------------------------------------------------
// string への追加書き込みを支援するクラス
//
// 文字列バッファを所有しない。
// std::sstream 使えばいいのでは。
// 書き込み量制限の機能を省いたので必要性が微妙に。
// (書き込み量制限はバッファ側が持っているべきかと)
//------------------------------------------------
class CStrWriter
{
protected:
	using string = std::string;
	using buf_t = std::shared_ptr<CStrBuf>;

public:
	CStrWriter() = delete;
	explicit CStrWriter(buf_t buf)
		: buf_ { buf }
		, depth_(0)
		, head_(true)
	{ }

public:
	auto get() const -> string const&;
	auto getBuf() const -> buf_t { return buf_; }

	auto is_full() const -> bool;

	// 出力メソッド
	void cat(char const* src);
	void cat(string const& src) { cat(src.c_str()); }
	void catln(char const* s) { cat(s); catCrlf(); }
	void catln(string const& s) { cat(s); catCrlf(); }
	void catCrlf();

	void cat(std::string_view const& source) {
		cat(source.data());
	}

	void cat(Utf8StringView const& source) {
		cat(as_native(source).data());
	}

	void catln(Utf8StringView const& source) {
		catln(as_native(source).data());
	}

	void catSize(std::size_t size);
	void catPtr(void const* ptr);
	void catDump(void const* data, size_t size);

	// 字下げを1段階深くする。
	void indent();

	// 字下げを1段階浅くする。
	void unindent();

private:
	void catDumpImpl(void const* data, size_t size);

private:
	buf_t buf_;
	std::size_t depth_;
	bool head_;
};

#endif
