#ifndef IG_HPIMOD_BASIS_H
#define IG_HPIMOD_BASIS_H

//#include "hsp3plugin.h"
//#include "hsp3plugin_custom.h"
#undef stat

#include <algorithm>

// デバッグ用
#include <assert.h>

#ifdef _DEBUG
# include <stdio.h>
# include <stdarg.h>
# define DbgArea /* empty */
# define dbgout(message, ...) hpimod_msgboxf<void>(message, __VA_ARGS__)//MessageBoxA(0, message, "hpi", MB_OK)
template<class T>
T hpimod_msgboxf(char const* sFormat, ...)
{
	static char stt_buffer[1024];
	va_list arglist;
	va_start(arglist, sFormat);
	vsprintf_s(stt_buffer, 1024 - 1, sFormat, arglist);
	va_end(arglist);
	MessageBoxA(nullptr, stt_buffer, "hpi debug", MB_OK);
}
#else
# define DbgArea if ( false )
# define dbgout(message, ...) ((void)0)
template<class T> T hpimod_msgboxf(char const* sFormat, ...) {}
#endif

namespace hpimod
{

// consts
	
// 次元数
static size_t const ArrayDimMax = 4;

// 配列の添字に用いる括弧
#define BracketIdxL "("
#define BracketIdxR ")"

// 型タイプ値
static const int HSPVAR_FLAG_COMOBJ = HSPVAR_FLAG_COMSTRUCT;
static const int HSPVAR_FLAG_VARIANT = 7;

// types
using vartype_t = unsigned short;
using varmode_t = signed short;
using label_t = unsigned short const*;		// a.k.a. HSPVAR_LABEL
using csptr_t = unsigned short const*;
using stdat_t = STRUCTDAT const*;
using stprm_t = STRUCTPRM const*;

using operator_t = void(*)(PDAT*, void const*);		// HspVarProc の演算関数
using hsp3DebugFunc_t = BOOL(CALLBACK*)(HSP3DEBUG*, int, int, int);

// デバッグウィンドウへの通知ID
enum DebugNotice
{
	DebugNotice_Stop = 0,		// 停止したとき (stop, wait, await, assert など)
	DebugNotice_Logmes,			// logmes 命令が呼ばれたとき (ctx->stmp で文字列を参照できる)
};

// functions

// 短縮名
static HspVarProc* getHvp(vartype_t flag) { return exinfo->HspFunc_getproc(flag); }
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

static size_t cntLabels() { return ctx->hsphed->max_ot / sizeof(int); }
static size_t cntStDats() { return ctx->hsphed->max_finfo / sizeof(STRUCTDAT); }
static size_t cntStPrms() { return ctx->hsphed->max_minfo / sizeof(STRUCTPRM); }

namespace detail
{
	// (failure: -1)
	// todo: std::search に帰着できそう？
	template<class TIter, class TValue>
	int indexOf(TIter begin, TIter end, TValue const& val)
	{
#if 0
		for (int i = 0; begin != end; ++begin, ++i) {
			if (*begin == val) return i;
		}
		return -1;
#else
		auto const iter = std::search(begin, end, &val, &val + 1);
		return (iter != end
			? std::distance(begin, iter)
			: -1);
#endif
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

// ot-index の検索 (failure: -1)
static int findOTIndex(label_t lb)
{
	return detail::indexOf(ctx->mem_ot, ctx->mem_ot + cntLabels(), getOTPtr(lb));
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

static stprm_t FlexValue_getModuleTag(FlexValue const* self) {	// structtag を持つ stprm
	return getSTRUCTPRM(self->customid); }
static stdat_t FlexValue_getModule(FlexValue const* self) {	// module である stdat
	return STRUCTPRM_getStDat(FlexValue_getModuleTag(self));
}
static bool FlexValue_isClone(FlexValue const* self) { return (self->type == FLEXVAL_TYPE_CLONE); }

// prmstack におけるメンバ stprm の領域へのポインタ
template<typename TVoid = void,  bool bConst = std::is_const<TVoid>::value>
static auto Prmstack_getMemberPtr(TVoid* self, stprm_t stprm) -> detail::const_iff_t<void, bConst>*
{ return static_cast<detail::const_iff_t<char, bConst>*>(self) + stprm->offset; }

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

static void* PVal_getPtr(PVal* pval) { return getHvp(pval->flag)->GetPtr(pval); }
static void const* PVal_getPtr(PVal const* pval) {
	// 実体ポインタを得るだけなのでおそらく安全
	return PVal_getPtr(const_cast<PVal*>(pval));
}
template<typename TPVal>
static auto PVal_getPtr(TPVal* pval, APTR aptr) -> detail::const_iff_t<void, std::is_const<TPVal>::value>*
{
	static_assert(std::is_same<std::remove_cv_t<TPVal>, PVal>::value, "typename TPVal must be PVal or PVal const.");

	APTR const bak = pval->offset;
	const_cast<PVal*>(pval)->offset = aptr;
	auto const result = PVal_getPtr(pval);
	const_cast<PVal*>(pval)->offset = bak;
	return result;
}

#if 0
// 使った覚えがない

static int const HspTrue = 1;
static int const HspFalse = 0;
static int HspBool(bool b) { return b ? HspTrue : HspFalse; }

static stdat_t getSTRUCTDAT(stprm_t stprm)
{
	stprm_t const root = ctx->mem_minfo;

	size_t i = stprm - root;
	for (; root[i].offset != 0; --i)	// stprm の直前の、offset == 0 となるものを探す
		;

	// prmindex == i となる stdat を探す
	stdat_t stdat = ctx->mem_finfo;
	for (; stdat->prmindex != i; ++stdat)
		;

	return stdat;
}
#endif

}; // namespace hpimod


#endif