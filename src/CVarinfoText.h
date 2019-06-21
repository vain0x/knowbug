#ifndef IG_CLASS_VARINFO_TEXT_H
#define IG_CLASS_VARINFO_TEXT_H

#include "main.h"
#include "module/CStrWriter.h"
#include "CVardataString.h"
#include "HspObjectPath.h"

class VTNodeModule;
class HspStaticVars;

namespace WrapCall
{
	struct ModcmdCallInfo;
}

// 変数データテキスト生成クラス
class CVarinfoText
{
public:
	CVarinfoText(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspStaticVars& static_vars);

	void add(HspObjectPath const& path);
	void add_module(HspObjectPath::Module const& path);
	void add_static_var(HspObjectPath::StaticVar const& path);

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

	auto create_lineform_writer() const->CVardataStrWriter;
	auto create_treeform_writer() const->CVardataStrWriter;

private:
	CStrWriter writer_;

	hpiutil::DInfo const& debug_segment_;
	HspObjects& objects_;
	HspStaticVars& static_vars_;
};

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

class HspObjectWriter::TableForm
	: public HspObjectWriter
{
public:
	TableForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

private:
	CVarinfoText& varinf_;
};

class HspObjectWriter::BlockForm
	: public HspObjectWriter
{
public:
	BlockForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

private:
	CVarinfoText& varinf_;
};

class HspObjectWriter::FlowForm
	: public HspObjectWriter
{
public:
	FlowForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf);

private:
	CVarinfoText& varinf_;
};

extern auto stringizePrmlist(stdat_t stdat) -> string;
extern auto stringizeVartype(PVal const* pval) -> string;

#endif
