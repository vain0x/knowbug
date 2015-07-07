#ifndef IG_HPIMOD_BASIS_H
#define IG_HPIMOD_BASIS_H

//#include "hsp3plugin_custom.h"

#include <vector>
#include <iterator>
#include <algorithm>
#include <cassert>

#ifdef _DEBUG
# include <stdio.h>
# include <stdarg.h>
# define DbgArea /* empty */
# define dbgout(message, ...) hpimod::Detail::hpimod_msgboxf(__FILE__, __LINE__, __FUNCTION__, message, __VA_ARGS__)
namespace hpimod { namespace Detail
{
static void hpimod_msgboxf(char const* _curfile, int _curline, char const* _curfunc, char const* format, ...)
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

namespace hpimod {
namespace Detail {

//find the value and return its offset, or -1.
template<class TIter, class TValue>
static auto indexOf(TIter begin, TIter end, TValue const& val) -> typename std::iterator_traits<TIter>::difference_type
{
	auto const iter = std::find(begin, end, val);
	return (iter != end ? std::distance(begin, iter) : -1);
}

//type T const (if flag = true), or T non-const.
template<typename T, bool flag>
using const_iff_t = std::conditional_t<flag, std::add_const_t<T>, std::remove_const_t<T>>;

} //namepsace Detail

static size_t const ArrayDimMax = 4;

// 配列の添字に用いる括弧
#define BracketIdxL "("
#define BracketIdxR ")"

static int const HSPVAR_FLAG_COMOBJ = HSPVAR_FLAG_COMSTRUCT;
static int const HSPVAR_FLAG_VARIANT = 7;

using vartype_t = unsigned short;
using varmode_t = signed short;
using label_t = unsigned short const*; // a.k.a. HSPVAR_LABEL
using csptr_t = unsigned short const*;
using stdat_t = STRUCTDAT const*;
using stprm_t = STRUCTPRM const*;

//HspVarProcの演算関数 (redefne されてない型)
using operator_t = void(*)(PDAT*, void const*);

//デバッグウィンドウのコールバック関数
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

static ptrdiff_t getOTPtr(label_t lb) { return (lb - ctx->mem_mcs); }
static csptr_t getCSPtr(ptrdiff_t otptr) { return &ctx->mem_mcs[otptr]; }
static label_t getLabel(ptrdiff_t otindex) { return getCSPtr(ctx->mem_ot[otindex]); }
static stdat_t getSTRUCTDAT(ptrdiff_t subid) { return &ctx->mem_finfo[subid]; }
static stprm_t getSTRUCTPRM(ptrdiff_t subid) { return &ctx->mem_minfo[subid]; }
static PVal*   getPVal(ptrdiff_t idx) { return &ctx->mem_var[idx]; }
static MEM_HPIDAT* getHPIDAT(ptrdiff_t idx)
{
	auto const mem_hpi = reinterpret_cast<MEM_HPIDAT*>(uintptr_t(ctx->hsphed) + ctx->hsphed->pt_hpidat);
	return &mem_hpi[idx];
}
static LIBDAT* getLIBDAT(int idx)
{
	auto const mem_linfo = reinterpret_cast<LIBDAT*>(uintptr_t(ctx->hsphed) + ctx->hsphed->pt_linfo);
	return &mem_linfo[idx];
}

static size_t cntSttVars() { return ctx->hsphed->max_val; }
static size_t cntLabels() { return ctx->hsphed->max_ot / sizeof(int); }
static size_t cntStDats() { return ctx->hsphed->max_finfo / sizeof(STRUCTDAT); }
static size_t cntStPrms() { return ctx->hsphed->max_minfo / sizeof(STRUCTPRM); }
static size_t cntHpis()   { return ctx->hsphed->max_hpi / sizeof(HPIDAT); }
static size_t cntLibs()   { return ctx->hsphed->max_linfo / sizeof(LIBDAT); }

// 静的変数の検索 (failure: nullptr)
static PVal* seekSttVar(char const* name)
{
	int const iVar = exinfo->HspFunc_seekvar(name);
	return (iVar >= 0) ? getPVal(iVar) : nullptr;
}

static char const* nameFromStaticVar(PVal const* pval) {
	ptrdiff_t const idx = pval - ctx->mem_var;
	return (ptrdiff_t(0) <= idx && idx < ctx->hsphed->max_val)
		? exinfo->HspFunc_varname(static_cast<int>(idx))
		: nullptr;
}

// ot-index の検索 (failure: -1)
static int findOTIndex(label_t lb)
{
	ptrdiff_t const idx = Detail::indexOf(ctx->mem_ot, ctx->mem_ot + cntLabels(), getOTPtr(lb));
	return (0 <= idx && static_cast<uintptr_t>(idx) < cntLabels() ? static_cast<int>(idx) : -1) ;
}

// stprm index の検索 (failure: -1)
static int findStPrmIndex(stprm_t stprm)
{
	ptrdiff_t const idx = stprm - ctx->mem_minfo;
	return (0 <= idx && static_cast<uintptr_t>(idx) < cntStPrms()) ? static_cast<int>(idx) : -1;
}

static char const* STRUCTDAT_getName(stdat_t self) { return &ctx->mem_mds[self->nameidx]; }
static stprm_t STRUCTDAT_getStPrm(stdat_t self) { return getSTRUCTPRM(self->prmindex); }
static stprm_t STRUCTDAT_getStPrmEnd(stdat_t self) { return STRUCTDAT_getStPrm(self) + self->prmmax; }
static stdat_t STRUCTPRM_getStDat(stprm_t self) { return getSTRUCTDAT(self->subid); }

//ユーザ定義コマンドに対応する STRUCTDAT
static bool STRUCTDAT_isSttmOrFunc(stdat_t self)
{
	return (self->index == STRUCTDAT_INDEX_FUNC || self->index == STRUCTDAT_INDEX_CFUNC);
}

//structtag を持つ stprm
static stprm_t FlexValue_getModuleTag(FlexValue const* self)
{
	return getSTRUCTPRM(self->customid);
}
//モジュールクラスのSTRUCTDAT
static stdat_t FlexValue_getModule(FlexValue const* self)
{
	return STRUCTPRM_getStDat(FlexValue_getModuleTag(self));
}
static bool FlexValue_isClone(FlexValue const* self)
{
	return (self->type == FLEXVAL_TYPE_CLONE);
}

// prmstack におけるメンバ stprm の実データへのポインタ
template<typename TVoid = void,  bool bConst = std::is_const<TVoid>::value>
static auto Prmstack_getMemberPtr(TVoid* self, stprm_t stprm) -> Detail::const_iff_t<void, bConst>*
{
	return static_cast<Detail::const_iff_t<char, bConst>*>(self) + stprm->offset;
}

static size_t PVal_maxDim(PVal const* pval)
{
	size_t i = 0;
	for ( ; i < ArrayDimMax && pval->len[i + 1] > 0; ++i ) {}
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
static std::vector<int> PVal_indexesFromAptr(PVal const* pval, APTR aptr)
{
	size_t const dim = PVal_maxDim(pval);
	std::vector<int> indexes(dim);
	for ( size_t i = 0; i < dim; ++i ) {
		indexes[i] = aptr % pval->len[i + 1];
		aptr /= pval->len[i + 1];
	}
	return std::move(indexes);
}
static bool PVal_isStandardArray(PVal const* pval)
{
	if ( pval->len[1] == 1 && pval->len[2] == 0 ) return false;
	auto const hvp = getHvp(pval->flag);
	return (hvp->support & (HSPVAR_SUPPORT_FIXEDARRAY | HSPVAR_SUPPORT_FLEXARRAY));
}

static PDAT* PVal_getPtr(PVal* pval) { return getHvp(pval->flag)->GetPtr(pval); }
static PDAT const* PVal_getPtr(PVal const* pval)
{
	//実体ポインタを得るだけなのでおそらく const に違反しない
	return PVal_getPtr(const_cast<PVal*>(pval));
}

template<typename TPVal>
static auto PVal_getPtr(TPVal* pval, APTR aptr) -> Detail::const_iff_t<PDAT, std::is_const<TPVal>::value>*
{
	if ( aptr == 0 ) return pval->pt;

	APTR const bak = pval->offset;
	const_cast<PVal*>(pval)->offset = aptr;
	auto const result = PVal_getPtr(pval);
	const_cast<PVal*>(pval)->offset = bak;
	return result;
}

// HSPの論理値
static int const HspTrue = 1;
static int const HspFalse = 0;
static int HspBool(bool b) { return b ? HspTrue : HspFalse; }

static bool isDebugMode() { return ctx->hspstat & HSPSTAT_DEBUG; }

} //namespace hpimod

#endif
