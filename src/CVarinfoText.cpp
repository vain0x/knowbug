// 変数データテキスト生成クラス

#include <numeric>	// for accumulate
#include <algorithm>

#include "module/strf.h"
#include "module/ptr_cast.h"
#include "module/CStrBuf.h"
#include "hpiutil/dinfo.hpp"

#include "main.h"
#include "DebugInfo.h"
#include "CVarinfoText.h"
#include "CVardataString.h"
#include "VarTreeNodeData.h"
#include "HspObjectPath.h"
#include "HspObjects.h"
#include "HspStaticVars.h"

#ifdef with_WrapCall
# include "WrapCall//WrapCall.h"
# include "WrapCall/ModcmdCallInfo.h"
using WrapCall::ModcmdCallInfo;
#endif

static auto const MAX_TEXT_DEPTH = std::size_t{ 8 };
static auto const MAX_TEXT_LENGTH = std::size_t{ 0x8000 };

CVarinfoText::CVarinfoText(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspStaticVars& static_vars)
	: writer_(std::make_shared<CStrBuf>())
	, debug_segment_(debug_segment)
	, objects_(objects)
	, static_vars_(static_vars)
{
	writer_.getBuf()->limit(MAX_TEXT_LENGTH);
}

auto CVarinfoText::getString() const -> string const&
{
	return getBuf()->get();
}

auto CVarinfoText::getStringMove() -> string&&
{
	return getBuf()->getMove();
}

auto CVarinfoText::create_lineform_writer() const -> CVardataStrWriter {
	auto writer = std::make_unique<CLineformedWriter>(getBuf(), MAX_TEXT_DEPTH);
	return CVardataStrWriter{ std::move(writer), debug_segment_, objects_, static_vars_ };
}

auto CVarinfoText::create_treeform_writer() const -> CVardataStrWriter {
	auto writer = std::make_unique<CTreeformedWriter>(getBuf(), MAX_TEXT_DEPTH);
	return CVardataStrWriter{ std::move(writer), debug_segment_, objects_, static_vars_ };
}

auto CVarinfoText::to_table_form() -> HspObjectWriter::TableForm {
	return HspObjectWriter::TableForm{ objects_, getWriter(), *this };
}

auto CVarinfoText::to_block_form() -> HspObjectWriter::BlockForm {
	return HspObjectWriter::BlockForm{ objects_, getWriter(), *this };
}

auto CVarinfoText::to_flow_form() -> HspObjectWriter::FlowForm {
	return HspObjectWriter::FlowForm{ objects_, getWriter(), *this };
}

void CVarinfoText::add(HspObjectPath const& path) {
	to_table_form().accept(path);
}

//------------------------------------------------
// 変数データから生成
//------------------------------------------------
void CVarinfoText::addVar( PVal* pval, char const* name )
{
	auto const hvp = hpiutil::varproc(pval->flag);
	int bufsize;
	void const* const pMemBlock =
		hvp->GetBlockSize(pval, ptr_cast<PDAT*>(pval->pt), ptr_cast<int*>(&bufsize));

	// 変数に関する情報
	getWriter().catln(strf("変数名: %s", name));
	getWriter().catln(strf("変数型: %s", stringizeVartype(pval)));
	getWriter().catln(
		strf("アドレス: %p, %p"
			, cptr_cast<void*>(pval->pt), cptr_cast<void*>(pval->master)
			));
	getWriter().catln(
		strf("サイズ: %d / %d [byte]"
			, pval->size, bufsize
			));
	getWriter().catCrlf();

	// 変数の内容に関する情報
	create_treeform_writer()
		.addVar(name, pval);
	getWriter().catCrlf();

	// メモリダンプ
	getWriter().catDump(pMemBlock, static_cast<size_t>(bufsize));
}

//------------------------------------------------
// システム変数データから生成
//------------------------------------------------
void CVarinfoText::addSysvar(hpiutil::Sysvar::Id id)
{
	getWriter().catln(strf("変数名: %s\t(システム変数)", hpiutil::Sysvar::List[id].name));
	getWriter().catCrlf();

	create_treeform_writer()
		.addSysvar(id);

	getWriter().catCrlf();

	// メモリダンプ
	auto dump = hpiutil::Sysvar::tryDump(id);
	if ( dump.first ) {
		getWriter().catDump(dump.first, dump.second);
	}
}

#if with_WrapCall
//------------------------------------------------
// 呼び出しデータから生成
//
// @prm prmstk: nullptr => 引数未確定
//------------------------------------------------
void CVarinfoText::addCall(ModcmdCallInfo const& callinfo)
{
	auto const stdat = callinfo.stdat;
	addCallSignature(callinfo, stdat);
	getWriter().catCrlf();

	auto prmstk_safety = callinfo.tryGetPrmstk();
	create_treeform_writer()
		.addCall(stdat, prmstk_safety);

	auto const prmstk = prmstk_safety.first;
	if ( prmstk ) {
		getWriter().catCrlf();
		getWriter().catDump(prmstk, static_cast<size_t>(stdat->size));
	}
}

void CVarinfoText::addCallSignature(ModcmdCallInfo const& callinfo, stdat_t stdat)
{
	auto const& name = callinfo.name();
	getWriter().catln(strf("関数名: %s (%s)"
		, name, callinfo.callerPos.toString()));

	// シグネチャ
	getWriter().catln(strf("仮引数: (%s)", stringizePrmlist(stdat)));
}

#endif

//**********************************************************
//        概観の追加
//**********************************************************

//------------------------------------------------
// [add] システム変数概観
//------------------------------------------------
void CVarinfoText::addSysvarsOverview()
{
	using namespace hpiutil;

	getWriter().catln("[システム変数]");

	for ( auto i = 0; i < Sysvar::Count; ++i ) {
		getWriter().cat(Sysvar::List[i].name);
		getWriter().cat("\t= ");
		create_lineform_writer()
			.addSysvar(static_cast<Sysvar::Id>(i));
		getWriter().catCrlf();
	}
}

#ifdef with_WrapCall
//------------------------------------------------
// [add] 呼び出し概観
//
// depends on WrapCall
//------------------------------------------------
void CVarinfoText::addCallsOverview()
{
	getWriter().catln("[呼び出し履歴]");

	for ( auto& callinfo : WrapCall::getCallInfoRange() ) {
		create_lineform_writer()
			.addCall(callinfo->stdat, callinfo->tryGetPrmstk());
		getWriter().catCrlf();
	}
}
#endif

//------------------------------------------------
// [add] 全般概観
//------------------------------------------------
void CVarinfoText::addGeneralOverview()
{
	getWriter().catln("[全般]");
	for ( auto&& kv : g_dbginfo->fetchGeneralInfo() ) {
		auto const isSysvar =
			hpiutil::Sysvar::trySeek(kv.first.c_str()) != hpiutil::Sysvar::MAX;
		if ( isSysvar ) continue;

		getWriter().catln(kv.first + "\t= " + kv.second);
	}
}

//------------------------------------------------
// 仮引数リストの文字列
//------------------------------------------------
auto stringizePrmlist(stdat_t stdat) -> string
{
	auto s = string { "" };
	for ( auto const& stprm : hpiutil::STRUCTDAT_params(stdat) ) {
		if ( ! s.empty() ) s += ", ";
		s += hpiutil::nameFromMPType(stprm.mptype);
	}
	return s;
}

static auto typeQualifierStringFromVarmode(varmode_t mode) -> char const*
{
	return
		(mode == HSPVAR_MODE_NONE  ) ? "!" :
		(mode == HSPVAR_MODE_MALLOC) ? ""  :
		(mode == HSPVAR_MODE_CLONE ) ? "&" :
		"<err>";
}

//------------------------------------------------
// 変数の型を表す文字列
//------------------------------------------------
auto stringizeVartype(PVal const* pval) -> string
{
	auto const maxDim = hpiutil::PVal_maxDim(pval);

	auto const arrayType =
		(maxDim == 0) ? "(empty)" :
		(maxDim == 1) ? hpiutil::stringifyArrayIndex({ pval->len[1] }) :
		strf("%s (%d in total)"
			, hpiutil::stringifyArrayIndex
				(std::vector<int>(&pval->len[1], &pval->len[1] + maxDim))
			, hpiutil::PVal_cntElems(pval))
		;

	return strf("%s%s %s"
		, hpiutil::varproc(pval->flag)->vartype_name
		, typeQualifierStringFromVarmode(pval->mode)
		, arrayType
		);
}

// -----------------------------------------------
// テーブルフォーム
// -----------------------------------------------

HspObjectWriter::HspObjectWriter(HspObjects& objects, CStrWriter& writer)
	: Visitor(objects)
	, writer_(writer)
{
}

HspObjectWriter::TableForm::TableForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf)
	: HspObjectWriter(objects, writer)
	, varinf_(varinf)
{
}

void HspObjectWriter::TableForm::on_module(HspObjectPath::Module const& path) {
	auto&& w = writer();
	auto&& objects_ = objects();
	auto&& name = path.name(objects_);

	w.cat("[");
	w.cat(name.data());
	w.catln("]");

	for (auto i = std::size_t{}; i < path.child_count(objects_); i++) {
		auto&& child_path = path.child_at(i, objects_);
		varinf_.to_block_form().accept(*child_path);
	}
}

void HspObjectWriter::TableForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto pval = objects().static_var_to_pval(path.static_var_id());
	auto name = path.name(objects());

	// 新APIが実装済みのケース
	if (path.type(objects()) == HspType::Int) {
		auto const hvp = hpiutil::varproc(pval->flag);
		int bufsize;
		void const* const pMemBlock =
			hvp->GetBlockSize(pval, ptr_cast<PDAT*>(pval->pt), ptr_cast<int*>(&bufsize));

		// 変数に関する情報
		writer().catln(strf("変数名: %s", name));
		writer().catln(strf("変数型: %s", stringizeVartype(pval)));
		writer().catln(
			strf("アドレス: %p, %p"
				, cptr_cast<void*>(pval->pt), cptr_cast<void*>(pval->master)
				));
		writer().catln(
			strf("サイズ: %d / %d [byte]"
				, pval->size, bufsize
				));
		writer().catCrlf();

		// 変数の内容に関する情報
		varinf_.to_block_form().accept_children(path);
		// writer().cat(name.data());
		// writer().cat(" = ");
		// add(*path.child_at(0, objects_));
		writer().catCrlf();

		// メモリダンプ
		writer().catDump(pMemBlock, static_cast<size_t>(bufsize));
		return;
	}

	// 旧APIにフォールバック
	varinf_.addVar(pval, name.data());
}

// -----------------------------------------------
// ブロックフォーム
// -----------------------------------------------

HspObjectWriter::BlockForm::BlockForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf)
	: HspObjectWriter(objects, writer)
	, varinf_(varinf)
{
}

void HspObjectWriter::BlockForm::on_module(HspObjectPath::Module const& path) {
	auto&& name = path.name(objects());

	// (入れ子の)モジュールは名前だけ表示しておく
	writer().catln(name.data());
}

void HspObjectWriter::BlockForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto&& name = path.name(objects());
	auto short_name = hpiutil::nameExcludingScopeResolution(name.data());

	writer().cat(short_name.data());
	writer().cat("\t= ");

	varinf_.to_flow_form().accept(path);

	writer().catCrlf();
}

void HspObjectWriter::BlockForm::on_element(HspObjectPath::Element const& path) {
	auto& w = writer();
	auto&& name = path.name(objects());

	w.cat(name.data());
	w.cat("\t= ");

	varinf_.to_flow_form().accept(path);

	w.catCrlf();
}

// -----------------------------------------------
// フローフォーム
// -----------------------------------------------

HspObjectWriter::FlowForm::FlowForm(HspObjects& objects, CStrWriter& writer, CVarinfoText& varinf)
	: HspObjectWriter(objects, writer)
	, varinf_(varinf)
{
}

void HspObjectWriter::FlowForm::on_static_var(HspObjectPath::StaticVar const& path) {
	auto&& w = writer();
	auto type = path.type(objects());
	auto&& type_name = objects().type_name(type);
	auto child_count = path.child_count(objects());

	// FIXME: 多次元配列の表示を改善する

	w.cat("<");
	w.cat(type_name.data());
	w.cat(">[");

	for (auto i = std::size_t{}; i < child_count; i++) {
		auto&& child = path.child_at(i, objects());

		if (i != 0) {
			w.cat(", ");
		}

		accept(*child);
	}

	w.cat("]");
}

void HspObjectWriter::FlowForm::on_int(HspObjectPath::Int const& path) {
	writer().cat(strf("%d", path.value(objects())));
}
