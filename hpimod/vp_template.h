// HspVarProc のひな形

#ifndef IG_HSPVARPROC_TEMPLATE_H
#define IG_HSPVARPROC_TEMPLATE_H

#include "hsp3plugin_custom.h"

#if _DEBUG
 #include "mod_makepval.h"
#endif
#include <functional>

namespace hpimod
{

//------------------------------------------------
// 実体ポインタを得る
//
// @ pt が value[] である場合
//------------------------------------------------
template<typename vtTag>
static PDAT* HspVarTemplate_GetPtr(PVal* pval)
{
	static_assert(VtTraits::isNativeVartype<vtTag>::value, "valptr_t = value_t[] である変数型でのみ使用できる。");
	assert(PVal_supportArray(pval));
	return VtTraits::asPDAT<vtTag>( VtTraits::getValptr<vtTag>(pval) );
}

//------------------------------------------------
// 可変長型に特有の関数の、固定長型の場合
//------------------------------------------------
template<typename vtTag>
static int HspVarTemplate_GetSize(PDAT const* pdat)
{
	static_assert(VtTraits::isFixed<vtTag>::value, "固定長の変数型でのみ使用できる。");
	return VtTraits::basesize<vtTag>::value;
}

template<typename vtTag>
static void* HspVarTemplate_GetBlockSize(PVal* pval, PDAT* pdat, int* size)
{
	static_assert(VtTraits::isFixed<vtTag>::value, "固定長の変数型でのみ使用できる。");
	*size = VtTraits::basesize<vtTag>::value * PVal_cntElems(pval);
	return pdat;
}

template<typename vtTag>
static void HspVarTemplate_AllocBlock(PVal* pval, PDAT* pdat, int size)
{
	static_assert(VtTraits::isFixed<vtTag>::value, "固定長の変数型でのみ使用できる。");
	// do nothing
	return;
}

//------------------------------------------------
// 演算用関数の型変換
//------------------------------------------------
using typeRedefinedOperator_t = void(*)(PDAT*, PDAT const*);

static operator_t HspVarProcOperatorCast(typeRedefinedOperator_t op)
{
	return reinterpret_cast<operator_t>(op);
}

//------------------------------------------------
// 比較関数の具体化
//
// @ HspVar**_CmpI という1つの関数から演算関数を生成する。
// @ aftertype はその中で設定する必要がある。
//------------------------------------------------
using compare_func_t = int(*)(PDAT* pdat, PDAT const* val);

namespace Detail
{
template<compare_func_t CmpI, typename TCmpFunctor>
static void HspVarTemplate_CmpI(PDAT* pdat, PDAT const* val)
{
	static_assert(std::is_empty<TCmpFunctor>::value, "TCmpFunctor mustn't have nono-static member vars.");
	static TCmpFunctor comparer {};

	VtTraits::derefValptr<vtInt>(pdat) = HspBool( comparer(CmpI(pdat, val), 0) );

//	myhvp->aftertype = HSPVAR_FLAG_INT;
	return;
}
}

// 同値性のみ定義する
template<compare_func_t CmpI>
static void HspVarTemplate_InitCmpI_Equality(HspVarProc* hvp)
{
	hvp->EqI = HspVarProcOperatorCast( Detail::HspVarTemplate_CmpI< CmpI, std::equal_to<int> > );
	hvp->NeI = HspVarProcOperatorCast( Detail::HspVarTemplate_CmpI< CmpI, std::not_equal_to<int> > );
	return;
}

// 比較関数をすべて定義する
template<compare_func_t CmpI>
static void HspVarTemplate_InitCmpI_Full(HspVarProc* hvp)
{
	HspVarTemplate_InitCmpI_Equality< CmpI >(hvp);

	hvp->LtI   = HspVarProcOperatorCast(Detail::HspVarTemplate_CmpI< CmpI, std::less<int> > );
	hvp->GtI   = HspVarProcOperatorCast(Detail::HspVarTemplate_CmpI< CmpI, std::greater<int> > );
	hvp->LtEqI = HspVarProcOperatorCast(Detail::HspVarTemplate_CmpI< CmpI, std::less_equal<int> > );
	hvp->GtEqI = HspVarProcOperatorCast(Detail::HspVarTemplate_CmpI< CmpI, std::greater_equal<int> > );
	return;
}

} // namespace hpimod

#endif
