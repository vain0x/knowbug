
#pragma once

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include "vector_view.hpp"

#include "hpiutil_fwd.hpp"
#include "vartype_traits.hpp"
#include "Sysvar.hpp"

namespace hpiutil {

namespace detail {

template<typename T>
static auto make_vector_view(T* p, size_t count) -> std::vector_view<T>
{
	return std::vector_view<T>(p, count);
}

} // namespace detail

class DInfo;

static auto varproc(vartype_t vtype) -> HspVarProc*
{
	return exinfo->HspFunc_getproc(vtype);
}

static auto tryFindHvp(char const* name) -> HspVarProc*
{
	return exinfo->HspFunc_seekproc(name);
}

static auto tryFindSttVarName(PVal const* pval) -> char const*
{
	auto const idx = ptrdiff_t { pval - ctx->mem_var };
	return (ptrdiff_t { 0 } <= idx && idx < ctx->hsphed->max_val)
		? exinfo->HspFunc_varname(static_cast<int>(idx))
		: nullptr;
}

static auto strData(ptrdiff_t dsIndex) -> char const*
{
	return &ctx->mem_mds[dsIndex];
}

static double doubleData(ptrdiff_t dsIndex)
{
	return *reinterpret_cast<double*>(&ctx->mem_mds[dsIndex]);
}

static auto labels() -> std::vector_view<unsigned short>
{
	return detail::make_vector_view(ctx->mem_mcs, ctx->hsphed->max_ot / sizeof(int));
}

static auto staticVars() -> std::vector_view<PVal>
{
	return detail::make_vector_view(ctx->mem_var, ctx->hsphed->max_val);
}

static auto csinfo() -> std::vector_view<unsigned short>
{
	return detail::make_vector_view(ctx->mcs, ctx->hsphed->max_cs / sizeof(unsigned short));
}

static auto finfo() -> std::vector_view<STRUCTDAT>
{
	return detail::make_vector_view(ctx->mem_finfo, ctx->hsphed->max_finfo / sizeof(STRUCTDAT));
}

static auto minfo() -> std::vector_view<STRUCTPRM>
{
	return detail::make_vector_view(ctx->mem_minfo, ctx->hsphed->max_minfo / sizeof(STRUCTPRM));
}

static auto hpidat() -> std::vector_view<MEM_HPIDAT>
{
	return detail::make_vector_view
		( reinterpret_cast<MEM_HPIDAT*>(uintptr_t(ctx->hsphed) + ctx->hsphed->pt_hpidat)
		, ctx->hsphed->max_hpi / sizeof(HPIDAT));
}

static auto libdat() -> std::vector_view<LIBDAT>
{
	return detail::make_vector_view
		( reinterpret_cast<LIBDAT*>(uintptr_t(ctx->hsphed) + ctx->hsphed->pt_linfo)
		, ctx->hsphed->max_linfo / sizeof(LIBDAT));
}

static auto STRUCTDAT_params(stdat_t self) -> std::vector_view<STRUCTPRM const>
{
	return detail::make_vector_view(&minfo()[self->prmindex], self->prmmax);
}

static auto STRUCTDAT_name(stdat_t self) -> char const*
{
	return strData(self->nameidx);
}

static bool STRUCTDAT_isSttmOrFunc(stdat_t self)
{
	return (self->index == STRUCTDAT_INDEX_FUNC || self->index == STRUCTDAT_INDEX_CFUNC);
}

static auto STRUCTPRM_stdat(stprm_t self) -> stdat_t
{
	return &finfo()[self->subid];
}

static auto STRUCTPRM_miIndex(stprm_t self) -> ptrdiff_t
{
	return (minfo().begin() <= self && self < minfo().end())
		? std::distance(minfo().begin(), self)
		: (-1);
}

/**
構造体タグ

MPTYPE_STRUCTTAG のパラメータ。
スクリプト上においてモジュール名によって表されるコマンド。
//*/
static auto FlexValue_structTag(FlexValue const* self) -> stprm_t
{
	return &minfo()[self->customid];
}

// モジュールに対応する STRUCTDAT
static auto FlexValue_module(FlexValue const* self) -> stdat_t
{
	return STRUCTPRM_stdat(FlexValue_structTag(self));
}

static bool FlexValue_isClone(FlexValue const* self)
{
	return (self->type == FLEXVAL_TYPE_CLONE);
}

static auto Prmstack_memberPtr(void const* self, stprm_t stprm) -> void const*
{
	return static_cast<char const*>(self) + stprm->offset;
}

static auto Prmstack_memberPtr(void* self, stprm_t stprm) -> void*
{
	return const_cast<void*>(Prmstack_memberPtr(const_cast<void const*>(self), stprm));
}

static auto PVal_maxDim(PVal const* pval) -> size_t
{
	auto i = size_t { 0 };
	for ( ; i < ArrayDimMax && pval->len[i + 1] > 0; ++i ) {}
	return i;
}

static auto PVal_cntElems(PVal const* pval) -> size_t
{
	auto cnt = size_t { 1 };
	for ( auto i = size_t { 1 };; ++i ) {
		cnt *= pval->len[i];
		if ( i == ArrayDimMax || pval->len[i + 1] == 0 ) break;
	}
	return cnt;
}

static auto PVal_indexesFromAptr(PVal const* pval, APTR aptr) -> std::vector<int>
{
	auto const dim = PVal_maxDim(pval);
	auto indexes = std::vector<int>(dim);
	for ( auto i = size_t { 0 }; i < dim; ++i ) {
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

static auto PVal_getPtr(PVal* pval) -> PDAT*
{
	return varproc(pval->flag)->GetPtr(pval);
}

static auto PVal_getPtr(PVal* pval, APTR aptr) -> PDAT*
{
	if ( aptr == 0 ) return pval->pt;

	auto bak = pval->offset;
	pval->offset = aptr;
	auto result = PVal_getPtr(pval);
	pval->offset = std::move(bak);
	return result;
}

static auto PVal_getPtr(PVal const* pval, APTR aptr) -> PDAT const*
{
	// 実体ポインタを得るだけなので const な操作であるはず
	return PVal_getPtr(const_cast<PVal*>(pval), aptr);
}

static auto PVal_getPtr(PVal const* pval) -> PDAT const*
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
	using namespace internal_vartype_tags;
	switch ( vtype ) {
		case HSPVAR_FLAG_LABEL:  return fLabel (derefValptr<vtLabel >(pdat));
		case HSPVAR_FLAG_STR:    return fStr   (derefValptr<vtStr   >(pdat));
		case HSPVAR_FLAG_DOUBLE: return fDouble(derefValptr<vtDouble>(pdat));
		case HSPVAR_FLAG_INT:    return fInt   (derefValptr<vtInt   >(pdat));
		case HSPVAR_FLAG_STRUCT: return fStruct(derefValptr<vtStruct>(pdat));
		default: return fDef(pdat, vtype);
	}
}

} // namespace hpiutil
