#ifndef IG_HPIMOD_BASIS_H
#define IG_HPIMOD_BASIS_H

//#include "hsp3plugin_custom.h"
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

// 次元数
static size_t const ArrayDimMax = 4;

#define BracketIdxL "("
#define BracketIdxR ")"

#ifndef HSPVAR_FLAG_COMOBJ
static const int HSPVAR_FLAG_COMOBJ = HSPVAR_FLAG_COMSTRUCT;
#endif
static const int HSPVAR_FLAG_VARIANT = 7;

// types
typedef unsigned short vartype_t;
typedef   signed short varmode_t;
typedef unsigned short const* label_t;		// a.k.a. HSPVAR_LABEL
typedef unsigned short const* csptr_t;
typedef STRUCTDAT const* stdat_t;
typedef STRUCTPRM const* stprm_t;

typedef void(*operator_t)(PDAT*, void const*);		// HspVarProc の演算関数

// デバッグウィンドウへの通知ID
enum DebugNotice
{
	DebugNotice_None = 0,
	DebugNotice_Stop = 0,		// 停止したとき (stop, wait, await, assert など)
	DebugNotice_Logmes,			// logmes 命令が実行されたとき (ctx->smpに文字列)
	DebugNotice_MAX
};

// functions

// 短縮名
static HspVarProc* getHvp(vartype_t flag) { return exinfo->HspFunc_getproc(flag); }
static HspVarProc* seekHvp(char const* name) { return exinfo->HspFunc_seekproc(name); }

// ax ファイルへのアクセス

static int getOTPtr(label_t lb) {
	return lb - ctx->mem_mcs;
}
static label_t getLabel(int otindex) { return &ctx->mem_mcs[ctx->mem_ot[otindex]]; }
static stdat_t getSTRUCTDAT(int subid) { return &ctx->mem_finfo[subid]; }
static stprm_t getSTRUCTPRM(int subid) { return &ctx->mem_minfo[subid]; }
static PVal*   getPVal(int iVar) { return &ctx->mem_var[iVar]; }

namespace detail
{
	template<class T, class U>
	int indexOf(T const& begin, T const& end, U const& val)
	{
		for (int i = 0; begin + i != end; ++i) {
			if (*(begin + i) == val) return i;
		}
		return -1;
	}
}

// ot-index の検索 (failure: -1)
static int findOTIndex(label_t lb)
{
	return detail::indexOf(ctx->mem_ot, ctx->mem_ot + ctx->hsphed->max_ot/sizeof(int), getOTPtr(lb));
}

// stprm index の検索 (failure: -1)
static int findStPrmIndex(stprm_t stprm)
{
	int const i = stprm - ctx->mem_minfo;
	return (0 <= i && static_cast<size_t>(i) < ctx->hsphed->max_minfo/sizeof(stprm_t)) ? i : -1;
}

// 各構造体に対する便利関数

static char const* STRUCTDAT_getName(stdat_t self) { return &ctx->mem_mds[self->nameidx]; }
static stprm_t STRUCTDAT_getStPrm(stdat_t self) { return &ctx->mem_minfo[self->prmindex]; }
static stdat_t STRUCTPRM_getStDat(stprm_t self) { return &ctx->mem_finfo[self->subid]; }

static stdat_t FlexValue_getStDat(FlexValue const* self) { return &ctx->mem_finfo[self->customid]; }
static stprm_t FlexValue_getStPrm(FlexValue const* self) { return &ctx->mem_minfo[FlexValue_getStDat(self)->prmindex]; }

static size_t PVal_maxDim(PVal const* pval) {
	size_t i = 0;
	for (; i < ArrayDimMax; ++i) {
		if (pval->len[i + 1] == 0) { return i; }
	}
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

/*
// 使った覚えがない

int const HspTrue = 1;
int const HspFalse = 0;
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
//*/

}; // namespace hpimod


#endif
