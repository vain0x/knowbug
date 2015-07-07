// 変数データテキスト生成クラス

#include <numeric>	// for accumulate
#include <algorithm>

#include "module/strf.h"
#include "module/ptr_cast.h"
#include "module/CStrBuf.h"
#include "hpimod/stringization.h"

#include "main.h"
#include "DebugInfo.h"
#include "SysvarData.h"
#include "StaticVarTree.h"
#include "CVarinfoText.h"
#include "CVardataString.h"

#ifdef with_WrapCall
# include "VarTreeNodeData.h"
# include "WrapCall//WrapCall.h"
# include "WrapCall/ModcmdCallInfo.h"
using WrapCall::ModcmdCallInfo;
#endif

CVarinfoText::CVarinfoText()
	: writer_(std::make_shared<CStrBuf>())
{
	writer_.getBuf()->limit(std::max(0x100, g_config->maxLength));
}

string const& CVarinfoText::getString() const {
	return getBuf()->get();
}
string&& CVarinfoText::getStringMove() {
	return getBuf()->getMove();
}

//------------------------------------------------
// 変数データから生成
//------------------------------------------------
void CVarinfoText::addVar( PVal* pval, char const* name )
{
	auto const hvp = hpimod::getHvp(pval->flag);
	int bufsize;
	void const* const pMemBlock =
		hvp->GetBlockSize(pval, ptr_cast<PDAT*>(pval->pt), ptr_cast<int*>(&bufsize));

	// 変数に関する情報
	getWriter().catln(strf("変数名：%s", name));
	getWriter().catln(strf("変数型：%s", stringizeVartype(pval)));
	if ( g_config->showsVariableAddress ) {
		getWriter().catln(strf("アドレス：%p, %p", cptr_cast<void*>(pval->pt), cptr_cast<void*>(pval->master)));
	}
	if ( g_config->showsVariableSize ) {
		getWriter().catln(strf("サイズ：using %d of %d [byte]", pval->size, bufsize));
	}
	getWriter().catCrlf();

	// 変数の内容に関する情報
	{
		CVardataStrWriter::create<CTreeformedWriter>(getBuf())
			.addVar(name, pval);
	}
	getWriter().catCrlf();

	// メモリダンプ
	if ( g_config->showsVariableDump ) {
		 getWriter().catDump(pMemBlock, static_cast<size_t>(bufsize));
	}
}

//------------------------------------------------
// システム変数データから生成
//------------------------------------------------
void CVarinfoText::addSysvar(Sysvar::Id id)
{
	getWriter().catln(strf("変数名：%s\t(システム変数)", Sysvar::List[id].name));
	getWriter().catCrlf();
	{
		CVardataStrWriter::create<CTreeformedWriter>(getBuf())
			.addSysvar(id);
	}
	getWriter().catCrlf();

	// メモリダンプ
	if ( g_config->showsVariableDump ) {
		auto const&& dump = Sysvar::tryDump(id);
		if ( dump.first ) {
			getWriter().catDump(dump.first, dump.second);
		}
	}
}

#if with_WrapCall
//------------------------------------------------
// 呼び出しデータから生成
// 
// @prm prmstk: nullptr => 引数未確定
//------------------------------------------------
void CVarinfoText::addCall(ModcmdCallInfo::shared_ptr_type const& callinfo)
{
	auto const stdat = callinfo->stdat;
	addCallSignature(callinfo, stdat);
	getWriter().catCrlf();

	auto const&& prmstk_safety = callinfo->tryGetPrmstk();
	CVardataStrWriter::create<CTreeformedWriter>(getBuf())
			.addCall(stdat, prmstk_safety);

	auto const prmstk = prmstk_safety.first;
	if ( prmstk && g_config->showsVariableDump ) {
		getWriter().catCrlf();
		getWriter().catDump(prmstk, static_cast<size_t>(stdat->size));
	}
}

void CVarinfoText::addCallSignature(ModcmdCallInfo::shared_ptr_type const& callinfo, stdat_t stdat)
{
	auto const name = hpimod::STRUCTDAT_getName(stdat);
	getWriter().catln(
		(callinfo->fname == nullptr)
			? strf("関数名：%s", name)
			: strf("関数名：%s (#%d of %s)", name, callinfo->line + 1, callinfo->fname)
	);

	// シグネチャ
	getWriter().catln(strf("仮引数：(%s)", stringizePrmlist(stdat)));
}

//------------------------------------------------
// 返値データから生成
//------------------------------------------------
void CVarinfoText::addResult(shared_ptr<ResultNodeData> const& result)
{
	assert(!!result);
	addCallSignature(result->callinfo, result->callinfo->stdat);
	getWriter().catCrlf();
	getWriter().cat(result->treeformedString);
}

#endif

//**********************************************************
//        概観の追加
//**********************************************************
//------------------------------------------------
// [add] モジュール概観
//------------------------------------------------
void CVarinfoText::addModuleOverview(char const* name, StaticVarTree const& tree)
{
	getWriter().catln(strf("[%s]", name));

	tree.foreach(
		[&](StaticVarTree const& module) {
			// (入れ子の)モジュールは名前だけ表示しておく
			getWriter().catln(module.getName());
		},
		[&](string const& varname) {
			auto const shortName = hpimod::nameExcludingScopeResolution(varname);
			getWriter().cat(shortName + "\t= ");
			{
				CVardataStrWriter::create<CLineformedWriter>(getBuf())
					.addVar(varname.c_str(), hpimod::seekSttVar(varname.c_str()));
			}
			getWriter().catCrlf();
		}
	);
}

//------------------------------------------------
// [add] システム変数概観
//------------------------------------------------
void CVarinfoText::addSysvarsOverview()
{
	getWriter().catln("[システム変数]");

	for ( int i = 0; i < Sysvar::Count; ++i ) {
		getWriter().cat(Sysvar::List[i].name);
		getWriter().cat("\t= ");
		{
			CVardataStrWriter::create<CLineformedWriter>(getBuf())
				.addSysvar(static_cast<Sysvar::Id>(i));
		}
		getWriter().catCrlf();
	}
}

#ifdef with_WrapCall
//------------------------------------------------
// [add] 呼び出し概観
// 
// depends on WrapCall
//------------------------------------------------
void CVarinfoText::addCallsOverview(shared_ptr<ResultNodeData> const& pLastResult)
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
void CVarinfoText::addGeneralOverview() {
	getWriter().catln("[全般]");
	for ( auto&& kv : g_dbginfo->fetchGeneralInfo() ) {
		bool const isSysvar = (Sysvar::trySeek(kv.first.c_str()) != Sysvar::MAX);
		if ( isSysvar ) continue;

		getWriter().catln(kv.first + "\t= " + kv.second);
	}
}

//------------------------------------------------
// 仮引数リストの文字列
//------------------------------------------------
string stringizePrmlist(stdat_t stdat)
{
	string s = "";
	std::for_each(hpimod::STRUCTDAT_getStPrm(stdat), hpimod::STRUCTDAT_getStPrmEnd(stdat), [&](STRUCTPRM const& stprm) {
		if ( !s.empty() ) s += ", ";
		s += hpimod::nameFromMPType(stprm.mptype);
	});
	return s;
}

static char const* typeQualifierStringFromVarmode(varmode_t mode)
{
	return (mode == HSPVAR_MODE_NONE) ? "!" :
		(mode == HSPVAR_MODE_MALLOC) ? "" :
		(mode == HSPVAR_MODE_CLONE) ? "&" : "<err>";
}

//------------------------------------------------
// 変数の型を表す文字列
//------------------------------------------------
string stringizeVartype(PVal const* pval)
{
	size_t const maxDim = hpimod::PVal_maxDim(pval);

	string const arrayType =
		(maxDim == 0) ? "(empty)" :
		(maxDim == 1) ? hpimod::stringizeArrayIndex({ pval->len[1] }) :
		strf("%s (%d in total)",
			hpimod::stringizeArrayIndex(std::vector<int>(&pval->len[1], &pval->len[1] + maxDim)),
			hpimod::PVal_cntElems(pval))
	;

	return strf("%s%s %s",
		hpimod::getHvp(pval->flag)->vartype_name,
		typeQualifierStringFromVarmode(pval->mode),
		arrayType
	);
}
