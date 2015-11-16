
#pragma once

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <memory>
#include <cassert>

#include "hpiutil_fwd.hpp"
#include "vector_view.hpp"

namespace hpiutil {

namespace detail {

template<typename T>
static auto make_vector_view(T* p, size_t count) -> std::vector_view<T>
{
	return std::vector_view<T>(p, count);
}

} // namespace detail

static HspVarProc* varproc(vartype_t vtype)
{
	return exinfo->HspFunc_getproc(vtype);
}

static HspVarProc* tryFindHvp(char const* name)
{
	return exinfo->HspFunc_seekproc(name);
}

static char const* tryFindSttVarName(PVal const* pval)
{
	ptrdiff_t const idx = pval - ctx->mem_var;
	return (ptrdiff_t(0) <= idx && idx < ctx->hsphed->max_val)
		? exinfo->HspFunc_varname(static_cast<int>(idx))
		: nullptr;
}

static char const* strData(ptrdiff_t dsIndex)
{
	return &ctx->mem_mds[dsIndex];
}

static double doubleData(ptrdiff_t dsIndex)
{
	return *reinterpret_cast<double*>(&ctx->mem_mds[dsIndex]);
}

static std::vector_view<unsigned short> labels()
{
	return detail::make_vector_view(ctx->mem_mcs, ctx->hsphed->max_ot / sizeof(int));
}

static std::vector_view<PVal> staticVars()
{
	return detail::make_vector_view(ctx->mem_var, ctx->hsphed->max_val);
}

static std::vector_view<unsigned short> csinfo()
{
	return detail::make_vector_view(ctx->mcs, ctx->hsphed->max_cs / sizeof(unsigned short));
}

static std::vector_view<STRUCTDAT> finfo()
{
	return detail::make_vector_view(ctx->mem_finfo, ctx->hsphed->max_finfo / sizeof(STRUCTDAT));
}

static std::vector_view<STRUCTPRM> minfo()
{
	return detail::make_vector_view(ctx->mem_minfo, ctx->hsphed->max_minfo / sizeof(STRUCTPRM));
}

static std::vector_view<MEM_HPIDAT> hpidat()
{
	return detail::make_vector_view
		( reinterpret_cast<MEM_HPIDAT*>(uintptr_t(ctx->hsphed) + ctx->hsphed->pt_hpidat)
		, ctx->hsphed->max_hpi / sizeof(HPIDAT));
}

static std::vector_view<LIBDAT> libdat()
{
	return detail::make_vector_view
		( reinterpret_cast<LIBDAT*>(uintptr_t(ctx->hsphed) + ctx->hsphed->pt_linfo)
		, ctx->hsphed->max_linfo / sizeof(LIBDAT));
}

static PVal* seekSttVar(char const* name)
{
	int const index = exinfo->HspFunc_seekvar(name);
	return (index >= 0) ? &ctx->mem_var[index] : nullptr;
}

static std::vector_view<STRUCTPRM const> STRUCTDAT_params(stdat_t self)
{
	return detail::make_vector_view(&minfo()[self->prmindex], self->prmmax);
}

static char const* STRUCTDAT_name(stdat_t self)
{
	return strData(self->nameidx);
}

static bool STRUCTDAT_isSttmOrFunc(stdat_t self)
{
	return (self->index == STRUCTDAT_INDEX_FUNC || self->index == STRUCTDAT_INDEX_CFUNC);
}

static stdat_t STRUCTPRM_stdat(stprm_t self)
{
	return &finfo()[self->subid];
}

static ptrdiff_t STRUCTPRM_miIndex(stprm_t self)
{
	return ( minfo().begin() <= self && self < minfo().end() )
		? std::distance(minfo().begin(), self)
		: (-1);
}

/**
構造体タグ

MPTYPE_STRUCTTAG のパラメータ。
スクリプト上においてモジュール名によって表されるコマンド。
//*/
static stprm_t FlexValue_structTag(FlexValue const* self)
{
	return &minfo()[self->customid];
}

// モジュールに対応する STRUCTDAT
static stdat_t FlexValue_module(FlexValue const* self)
{
	return STRUCTPRM_stdat(FlexValue_structTag(self));
}

static bool FlexValue_isClone(FlexValue const* self)
{
	return (self->type == FLEXVAL_TYPE_CLONE);
}

static void const* Prmstack_memberPtr(void const* self, stprm_t stprm)
{
	return static_cast<char const*>(self) + stprm->offset;
}

static void* Prmstack_memberPtr(void* self, stprm_t stprm)
{
	return const_cast<void*>(Prmstack_memberPtr(const_cast<void const*>(self), stprm));
}

static size_t PVal_maxDim(PVal const* pval)
{
	size_t i = 0;
	for ( ; i < ArrayDimMax && pval->len[i + 1] > 0; ++i ) {}
	return i;
}

static size_t PVal_cntElems(PVal const* pval)
{
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
	return indexes;
}

static bool PVal_isStandardArray(PVal const* pval)
{
	if ( pval->len[1] == 1 && pval->len[2] == 0 ) return false;
	auto const hvp = varproc(pval->flag);
	return (hvp->support & (HSPVAR_SUPPORT_FIXEDARRAY | HSPVAR_SUPPORT_FLEXARRAY)) != 0;
}

static PDAT* PVal_getPtr(PVal* pval)
{
	return varproc(pval->flag)->GetPtr(pval);
}

static PDAT* PVal_getPtr(PVal* pval, APTR aptr)
{
	if ( aptr == 0 ) return pval->pt;

	APTR const bak = pval->offset;
	pval->offset = aptr;
	auto const result = PVal_getPtr(pval);
	pval->offset = bak;
	return result;
}

static PDAT const* PVal_getPtr(PVal const* pval, APTR aptr)
{
	// 実体ポインタを得るだけなので const な操作であるはず
	return PVal_getPtr(const_cast<PVal*>(pval), aptr);
}

static PDAT const* PVal_getPtr(PVal const* pval)
{
	return PVal_getPtr(pval, pval->offset);
}

// 値へのアクセス (なるべくC++の型を割り当てる)
template<typename R
	, typename FunLabel, typename FunStr, typename FunDouble, typename FunInt, typename FunStruct
	, typename FunDefault>
auto dispatchValue
	( PDAT const* pdat, vartype_t vtype
	, FunLabel&& fLabel, FunStr&& fStr, FunDouble&& fDouble, FunInt&& fInt, FunStruct&& fStruct
	, FunDefault&& fDef)
	-> R
{
	switch ( vtype ) {
		case HSPVAR_FLAG_LABEL:  return fLabel(*reinterpret_cast<label_t const*>(pdat));
		case HSPVAR_FLAG_STR:    return fStr(reinterpret_cast<char const*>(pdat));
		case HSPVAR_FLAG_DOUBLE: return fDouble(*reinterpret_cast<double const*>(pdat));
		case HSPVAR_FLAG_INT:    return fInt(*reinterpret_cast<int const*>(pdat));
		case HSPVAR_FLAG_STRUCT: return fStruct(*reinterpret_cast<FlexValue const*>(pdat));
		default: return fDef(pdat, vtype);
	}
}

} // namespace hpiutil
