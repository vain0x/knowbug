// 文字列への整形書き込み

#ifndef IG_CLASS_STRING_WRITER_H
#define IG_CLASS_STRING_WRITER_H

#include <string>
#include <stack>

class CStructedStrWriter;
class CTreeformedWriter;
class CLineformedWriter;

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

public:
	CStrWriter() = delete;
	explicit CStrWriter(string* buf)
		: buf_ { buf }
	{ }

public:
	string const& get() const { return *buf_; }
	string* getBuf() const { return buf_; }

	// 出力メソッド
	void cat(char const* src);
	void cat(string const& src) { cat(src.c_str()); }
	void catln(char const* s) { cat(s); catCrlf(); }
	void catln(string const& s) { cat(s); catCrlf(); }
	void catCrlf();
	void catDump(void const* data, size_t size);
private:
	void catDumpImpl(void const* data, size_t size);

	/*
public:
	// 書き込み量制限
	static char const* const stc_warning;	// "(too long)"
	static size_t const stc_warningLength = 10;	//= std::strlen(stc_warning);
	static size_t const stc_inifiniteLength = 0xFFFFFFFF;
	size_t getLimit() const { return lenLimit_; }
private:
	size_t lenLimit_;	// 残りの書き込み量制限
	//*/
private:
	string* buf_;
};

//------------------------------------------------
// 構造付き (treeform or lineform)
// 
// for CVardataStrWriter
// name, left-right Bracket の生成はどちらかしか使わないのに両方用意させる、やや富豪的仕様
// 標準配列変数の処理がいまいち
//------------------------------------------------
class CStructedStrWriter
	: public CStrWriter
{
	friend class CTreeformedWriter;
	friend class CLineformedWriter;

private:
	CStructedStrWriter(string* buf)
		: CStrWriter(buf)
		, lvNest_ { 0 }
	{ }

	// テンプレートメソッド
public:
	virtual bool isLineformed() const = 0;	// 裏技っぽい
	virtual void catLeaf(char const* name, char const* value) = 0;
	virtual void catLeafExtra(char const* name, char const* state) = 0;
	virtual void catNodeBegin(char const* name, char const* leftBracket) = 0;
	virtual void catNodeEnd(char const* rightBracket) = 0;
	virtual void catAttribute(char const* name, char const* value) = 0;

public:
	void catLeaf(string const& name, char const* value) { catLeaf(name.c_str(), value); }
	void catLeafExtra(string const& name, char const* state) { catLeafExtra(name.c_str(), state); }
	void catNodeBegin(string const& name, char const* leftBracket) { catNodeBegin(name.c_str(), leftBracket); }

	// ネスト管理
	void incNest() { ++lvNest_; }
	void decNest() { --lvNest_; }

	int getNest() const { return lvNest_; }
	bool inifiniteNesting() const { return lvNest_ > 64; }	// 無限ネストから保護

private:
	int lvNest_;

public:
	// 使用されない文字列 (デバッグ時に nullptr と区別したい)
	static char const* const stc_strUnused;
private:
	inline void assertUsingParameter(char const* from, char const* p) const {
		assert(p != nullptr && p != stc_strUnused);
	}
};

//------------------------------------------------
// ツリー状
//------------------------------------------------
class CTreeformedWriter
	: public CStructedStrWriter
{
public:
	explicit CTreeformedWriter(string* buf)
		: CStructedStrWriter(buf)
	{ }

public:
	bool isLineformed() const final override { return false; }

	// name = value
	void catLeaf(char const* name, char const* value) override
	{
		assertUsingParameter(__FUNCTION__, name);
		assertUsingParameter(__FUNCTION__, value);
		
		catIndent();
		cat(name); cat(" = "); cat(value);
		catCrlf();
	}
	// name : (state)
	void catLeafExtra(char const* name, char const* state) override
	{
		assertUsingParameter(__FUNCTION__, name);
		assertUsingParameter(__FUNCTION__, state);

		catIndent();
		cat(name); cat(" : ("); cat(state); cat(")");
		catCrlf();
	}
	// name:
	//     ....
	void catNodeBegin(char const* name, char const* leftBracket) override
	{
		assertUsingParameter(__FUNCTION__, name);

		catIndent();
		cat(name); cat(":");
		catCrlf();
		incNest();
	}
	void catNodeEnd(char const* rightBracket) override
	{
		decNest();
	}

	// .attr = value
	void catAttribute(char const* name, char const* value) override
	{
		assertUsingParameter(__FUNCTION__, name);
		assertUsingParameter(__FUNCTION__, value);

		catLeaf(("." + string(name)).c_str(), value);
	}
private:
	void catIndent() { cat(getIndent()); }
	string getIndent() const { return string(getNest(), '\t'); }
};

//------------------------------------------------
// 一行
//------------------------------------------------
class CLineformedWriter
	: public CStructedStrWriter
{
public:
	explicit CLineformedWriter(string* buf)
		: CStructedStrWriter(buf)
		, bFirstElem_({ true })	// sentinel elem
	{ }

	bool isLineformed() const final override { return true; }

	// value
	void catLeaf(char const* name, char const* value) override
	{
		assertUsingParameter(__FUNCTION__, value);

		catDelimiter();
		cat(value);
	}
	// (state)
	void catLeafExtra(char const* name, char const* state) override
	{
		assertUsingParameter(__FUNCTION__, state);

		catLeaf(name, strf("(%s)", state).c_str());
	}
	// leftBracket...rightBracket
	void catNodeBegin(char const* name, char const* leftBracket) override
	{
		assertUsingParameter(__FUNCTION__, leftBracket);

		catDelimiter();
		cat(leftBracket);
		incNest();
		bFirstElem_.push(true);
		return;
	}
	void catNodeEnd(char const* rightBracket) override
	{
		assertUsingParameter(__FUNCTION__, rightBracket);

		bFirstElem_.pop();
		decNest();
		cat(rightBracket);
	}
	// ignore
	void catAttribute(char const* name, char const* value) override
	{ }

public:
	void catDelimiter()
	{
		if ( getNest() ) {
			if ( !bFirstElem_.top() ) cat(", ");
			bFirstElem_.top() = false;
		}
		return;
	}

private:
	std::stack<bool> bFirstElem_;
};

#endif
