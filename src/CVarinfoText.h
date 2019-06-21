#ifndef IG_CLASS_VARINFO_TEXT_H
#define IG_CLASS_VARINFO_TEXT_H

#include "main.h"
#include "module/CStrWriter.h"
#include "CVardataString.h"
#include "HspObjectPath.h"

class CVarinfoText;
class VTNodeModule;
class HspStaticVars;

namespace WrapCall
{
	struct ModcmdCallInfo;
}

class HspObjectWriter
	: public HspObjectPath::Visitor
{
public:
	class TableForm;
	class BlockForm;
	class FlowForm;

private:
	CStrWriter& writer_;

public:
	explicit HspObjectWriter(HspObjects& objects, CStrWriter& writer);

	auto writer() -> CStrWriter& {
		return writer_;
	}
};

// テーブルフォーム。
// 文字列全体を使って、オブジェクトの詳細情報を表示する。
class HspObjectWriter::TableForm
	: public HspObjectWriter
{
public:
	TableForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

	void on_module(HspObjectPath::Module const& path) override;

	void on_static_var(HspObjectPath::StaticVar const& path) override;

private:
	CVarinfoText& varinf_;
};

// ブロックフォーム。
// 数行を使って、オブジェクトの情報を表示する。
// 構築した文字列は、テーブルフォームの内部に埋め込まれる。
class HspObjectWriter::BlockForm
	: public HspObjectWriter
{
public:
	BlockForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

	void on_module(HspObjectPath::Module const& path) override;

	void on_static_var(HspObjectPath::StaticVar const& path) override;

	void on_element(HspObjectPath::Element const& path) override;

private:
	CVarinfoText& varinf_;
};

// フローフォーム。
// オブジェクトの情報を簡易的に表示する。
// 基本的に改行を含まない。
class HspObjectWriter::FlowForm
	: public HspObjectWriter
{
public:
	FlowForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

	void on_int(HspObjectPath::Int const& path) override;

private:
	CVarinfoText& varinf_;
};

// 変数データテキスト生成クラス
class CVarinfoText
{
public:
	CVarinfoText(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspStaticVars& static_vars);

	auto to_table_form() -> HspObjectWriter::TableForm;

	auto to_block_form() -> HspObjectWriter::BlockForm;

	auto to_flow_form() -> HspObjectWriter::FlowForm;

	void add(HspObjectPath const& path);

	void addVar(PVal* pval, char const* name);
	void addSysvar(hpiutil::Sysvar::Id id);
#ifdef with_WrapCall
	void addCall(WrapCall::ModcmdCallInfo const& callinfo);
private:
	void addCallSignature(WrapCall::ModcmdCallInfo const& callinfo, stdat_t stdat);
public:
#endif

	void addSysvarsOverview();
#ifdef with_WrapCall
	void addCallsOverview();
#endif
	void addGeneralOverview();

	auto getString() const -> string const&;
	auto getStringMove() -> string&&;
private:
	auto getWriter() -> CStrWriter& { return writer_; }
	auto getBuf() const -> std::shared_ptr<CStrBuf> { return writer_.getBuf(); }

public:
	auto create_lineform_writer() const->CVardataStrWriter;
	auto create_treeform_writer() const->CVardataStrWriter;

private:
	CStrWriter writer_;

	hpiutil::DInfo const& debug_segment_;
	HspObjects& objects_;
	HspStaticVars& static_vars_;
};

extern auto stringizePrmlist(stdat_t stdat) -> string;
extern auto stringizeVartype(PVal const* pval) -> string;

#endif
