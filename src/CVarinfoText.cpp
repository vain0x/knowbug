// 変数データテキスト生成クラス

#include <numeric>	// for accumulate
#include <algorithm>

#include "module/strf.h"
#include "module/ptr_cast.h"
#include "module/CStrBuf.h"

#include "main.h"
#include "DebugInfo.h"
#include "CVarinfoText.h"
#include "CVardataString.h"
#include "VarTreeNodeData.h"

#ifdef with_WrapCall
# include "WrapCall//WrapCall.h"
# include "WrapCall/ModcmdCallInfo.h"
using WrapCall::ModcmdCallInfo;
#endif

static auto const MAX_TEXT_LENGTH = std::size_t{ 0x8000 };

CVarinfoText::CVarinfoText()
	: writer_(std::make_shared<CStrBuf>())
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
	CVardataStrWriter::create<CTreeformedWriter>(getBuf())
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

	CVardataStrWriter::create<CTreeformedWriter>(getBuf())
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
	CVardataStrWriter::create<CTreeformedWriter>(getBuf())
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

//------------------------------------------------
// 返値データから生成
//------------------------------------------------
void CVarinfoText::addResult(ResultNodeData const& result)
{
	addCallSignature(*result.callinfo, result.callinfo->stdat);
	getWriter().catCrlf();
	getWriter().cat(result.treeformedString);
}

#endif

//**********************************************************
//        概観の追加
//**********************************************************
//------------------------------------------------
// [add] モジュール概観
//------------------------------------------------
void CVarinfoText::addModuleOverview(char const* name, VTNodeModule const& tree)
{
	getWriter().catln(strf("[%s]", name));

	tree.foreach(
		[&](VTNodeModule const& module) {
			// (入れ子の)モジュールは名前だけ表示しておく
			getWriter().catln(module.name());
		},
		[&](string const& varname) {
			auto const shortName = hpiutil::nameExcludingScopeResolution(varname);
			getWriter().cat(shortName + "\t= ");
			CVardataStrWriter::create<CLineformedWriter>(getBuf())
				.addVar(varname.c_str(), hpiutil::seekSttVar(varname.c_str()));
			getWriter().catCrlf();
		}
	);
}

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
		CVardataStrWriter::create<CLineformedWriter>(getBuf())
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
void CVarinfoText::addCallsOverview(optional_ref<ResultNodeData const> pLastResult)
{
	getWriter().catln("[呼び出し履歴]");

	for ( auto& callinfo : WrapCall::getCallInfoRange() ) {
		CVardataStrWriter::create<CLineformedWriter>(getBuf())
			.addCall(callinfo->stdat, callinfo->tryGetPrmstk());
		getWriter().catCrlf();
	}

	if ( pLastResult ) {
		getWriter().cat("-> ");
		getWriter().catln(pLastResult->lineformedString);
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
