#ifndef IG_HPIMOD_BASIS_H
#define IG_HPIMOD_BASIS_H

//#include "hsp3plugin_custom.h"

#include <vector>
#include <algorithm>

// デバッグ用
#include <cassert>

#ifdef _DEBUG
# include <stdio.h>
# include <stdarg.h>
# define DbgArea /* empty */
# define dbgout(message, ...) hpimod::Detail::hpimod_msgboxf<void>(__FILE__, __LINE__, __FUNCTION__, message, __VA_ARGS__)
namespace hpimod { namespace Detail {
template<typename T> T hpimod_msgboxf(char const* _curfile, int _curline, char const* _curfunc, char const* format, ...)
{
	static char stt_buf[0x800];
	int const idx =
		sprintf_s(stt_buf, "ファイル：%s\r\n行番号：%d\r\n関数：%s\r\n\r\n", _curfile, _curline, _curfunc, format);
	assert(idx >= 0);
	va_list arglist;
	va_start(arglist, format);
	vsprintf_s(&stt_buf[idx], sizeof(stt_buf) - idx - 1, format, arglist);
	va_end(arglist);
	MessageBoxA(nullptr, stt_buf, "hpi debug", MB_OK);
}
} } //namespace hpimod::Detail
#else
# define DbgArea if ( false )
# define dbgout(message, ...) ((void)0)
#endif

namespace hpimod
{
// 配列の次元数
static size_t const ArrayDimMax = 4;

// 配列の添字に用いる括弧
#define BracketIdxL "("
#define BracketIdxR ")"

// 型タイプ値
static int const HSPVAR_FLAG_COMOBJ = HSPVAR_FLAG_COMSTRUCT;
static int const HSPVAR_FLAG_VARIANT = 7;

using vartype_t = unsigned short;
using varmode_t = signed short;
using label_t = unsigned short const*; // a.k.a. HSPVAR_LABEL
using csptr_t = unsigned short const*;
using stdat_t = STRUCTDAT const*;
using stprm_t = STRUCTPRM const*;

using operator_t = void(*)(PDAT*, void const*); // HspVarProc の演算関数 (redefne されてない型)
using hsp3DebugFunc_t = BOOL(CALLBACK*)(HSP3DEBUG*, int, int, int);

// デバッグウィンドウへの通知ID
enum DebugNotice {
	// 停止したとき (stop, wait, await, assert など)
	DebugNotice_Stop = 0,
	// logmes 命令が呼ばれたとき (ctx->stmp で文字列を参照できる)
	DebugNotice_Logmes = 1,
};

// 短縮名
static HspVarProc* getHvp(vartype_t vtype) { return exinfo->HspFunc_getproc(vtype); }
static HspVarProc* seekHvp(char const* name) { return exinfo->HspFunc_seekproc(name); }

// ax ファイルへのアクセス

static int getOTPtr(label_t lb) {
	return lb - ctx->mem_mcs;
}
static csptr_t getCSPtr(int otptr) { return &ctx->mem_mcs[otptr]; }
static label_t getLabel(int otindex) { return getCSPtr(ctx->mem_ot[otindex]); }
static stdat_t getSTRUCTDAT(int subid) { return &ctx->mem_finfo[subid]; }
static stprm_t getSTRUCTPRM(int subid) { return &ctx->mem_minfo[subid]; }
static PVal*   getPVal(int idx) { return &ctx->mem_var[idx]; }
static HPIDAT* getHPIDAT(int idx) {
	auto const mem_hpi = reinterpret_cast<HPIDAT*>(reinterpret_cast<char*>(ctx->hsphed) + ctx->hsphed->pt_hpidat);
	return &mem_hpi[idx];
}
static LIBDAT* getLIBDAT(int idx) {
	auto const mem_linfo = reinterpret_cast<LIBDAT*>(reinterpret_cast<char*>(ctx->hsphed) + ctx->hsphed->pt_linfo);
	return &mem_linfo[idx];
}

static size_t cntSttVars() { return ctx->hsphed->max_val; }
static size_t cntLabels() { return ctx->hsphed->max_ot / sizeof(int); }
static size_t cntStDats() { return ctx->hsphed->max_finfo / sizeof(STRUCTDAT); }
static size_t cntStPrms() { return ctx->hsphed->max_minfo / sizeof(STRUCTPRM); }
static size_t cntHpis()   { return ctx->hsphed->max_hpi / sizeof(HPIDAT); }
static size_t cntLibs()   { return ctx->hsphed->max_linfo / sizeof(LIBDAT); }

namespace Detail
{
	//(failure: -1)
	template<class TIter, class TValue>
	int indexOf(TIter begin, TIter end, TValue const& val)
	{
		auto const iter = std::search(begin, end, &val, &val + 1);
		return (iter != end ? std::distance(begin, iter) : -1);
	}

	// type T const (if flag = true), or T non-const.
	template<typename T, bool flag>
	using const_iff_t = std::conditional_t<flag, std::add_const_t<T>, std::remove_const_t<T>>;
}

// 静的変数の検索 (failure: nullptr)
static PVal* seekSttVar(char const* name)
{
	int const iVar = exinfo->HspFunc_seekvar(name);
	return (iVar >= 0) ? getPVal(iVar) : nullptr;
}

static char const* nameFromStaticVar(PVal const* pval) {
	auto const idx = pval - ctx->mem_var;
	return (0 <= idx && idx < ctx->hsphed->max_val)
		? exinfo->HspFunc_varname(idx)
		: nullptr;
}

// ot-index の検索 (failure: -1)
static int findOTIndex(label_t lb)
{
	return Detail::indexOf(ctx->mem_ot, ctx->mem_ot + cntLabels(), getOTPtr(lb));
}

// stprm index の検索 (failure: -1)
static int findStPrmIndex(stprm_t stprm)
{
	int const i = stprm - ctx->mem_minfo;
	return (0 <= i && static_cast<size_t>(i) < cntStPrms()) ? i : -1;
}

// 各構造体に対する便利関数

static char const* STRUCTDAT_getName(stdat_t self) { return &ctx->mem_mds[self->nameidx]; }
static stprm_t STRUCTDAT_getStPrm(stdat_t self) { return getSTRUCTPRM(self->prmindex); }
static stprm_t STRUCTDAT_getStPrmEnd(stdat_t self) { return STRUCTDAT_getStPrm(self) + self->prmmax; }
static stdat_t STRUCTPRM_getStDat(stprm_t self) { return getSTRUCTDAT(self->subid); }
static bool STRUCTDAT_isSttmOrFunc(stdat_t self) { return (self->index == STRUCTDAT_INDEX_FUNC || self->index == STRUCTDAT_INDEX_CFUNC); }

static stprm_t FlexValue_getModuleTag(FlexValue const* self) { // structtag を持つ stprm
	return getSTRUCTPRM(self->customid);
}
static stdat_t FlexValue_getModule(FlexValue const* self) { // module である stdat
	return STRUCTPRM_getStDat(FlexValue_getModuleTag(self));
}
static bool FlexValue_isClone(FlexValue const* self) { return (self->type == FLEXVAL_TYPE_CLONE); }

// prmstack におけるメンバ stprm の実データへのポインタ
template<typename TVoid = void,  bool bConst = std::is_const<TVoid>::value>
static auto Prmstack_getMemberPtr(TVoid* self, stprm_t stprm) -> Detail::const_iff_t<void, bConst>*
{
	return static_cast<Detail::const_iff_t<char, bConst>*>(self) + stprm->offset;
}

static size_t PVal_maxDim(PVal const* pval) {
	size_t i = 0;
	for (; i < ArrayDimMax && pval->len[i + 1] > 0; ++i)
		;
	return i;
}
static size_t PVal_cntElems(PVal const* pval) {
	size_t cnt = 1;
	for ( size_t i = 1;; ++i ) {
		cnt *= pval->len[i];
		if ( i == ArrayDimMax || pval->len[i + 1] == 0 ) break;
	}
	return cnt;
}
static std::vector<int> PVal_indexesFromAptr(PVal const* pval, APTR aptr) {
	size_t const dim = PVal_maxDim(pval);
	std::vector<int> indexes(dim);
	for ( size_t i = 0; i < dim; ++i ) {
		indexes[i] = aptr % pval->len[i + 1];
		aptr /= pval->len[i + 1];
	}
	return std::move(indexes);
}
static bool PVal_isStandardArray(PVal const* pval) {
	auto const hvp = getHvp(pval->flag);
	return (hvp->support & (HSPVAR_SUPPORT_FIXEDARRAY | HSPVAR_SUPPORT_FLEXARRAY))
		&& !(pval->len[1] == 1 && pval->len[2] == 0);
}

static PDAT* PVal_getPtr(PVal* pval) { return getHvp(pval->flag)->GetPtr(pval); }
static PDAT const* PVal_getPtr(PVal const* pval) {
	// 実体ポインタを得るだけなのでおそらく安全
	return PVal_getPtr(const_cast<PVal*>(pval));
}
template<typename TPVal>
static auto PVal_getPtr(TPVal* pval, APTR aptr) -> Detail::const_iff_t<PDAT, std::is_const<TPVal>::value>*
{
	static_assert(std::is_same<std::remove_cv_t<TPVal>, PVal>::value, "typename TPVal must be PVal or PVal const.");

	if ( aptr == 0 ) return pval->pt;

	APTR const bak = pval->offset;
	const_cast<PVal*>(pval)->offset = aptr;
	auto const result = PVal_getPtr(pval);
	const_cast<PVal*>(pval)->offset = bak;
	return result;
}

// HSP的論理値
static int const HspTrue = 1;
static int const HspFalse = 0;
static int HspBool(bool b) { return b ? HspTrue : HspFalse; }

// その他
static bool isDebugMode() { return ctx->hspstat & HSPSTAT_DEBUG; }

} //namespace hpimod

#endif
