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
// traits
//------------------------------------------------
// pt = TValue[] であるような素朴な型の tag
template<typename TValue, typename TMaster = void*>
struct NativeVartypeTag
{
	using value_t = TValue;
	using valptr_t = TValue*;
	using const_valptr_t = TValue const*;
	using master_t = TMaster;

	static int const basesize = sizeof(TValue);
};

// str 型のような型の tag
// master = TValue[], pt = master[0] となる。
template<typename TValue, int TBasesize>
struct StrLikeVartypeTag
{
	using value_t  = TValue;
	using valptr_t = TValue;
	using const_valptr_t = TValue const;

	using master_t = TValue*;

	static int const basesize = TBasesize;
};

// 組み込み型の変数型タグ

using label_tag  = NativeVartypeTag<label_t>;
using double_tag = NativeVartypeTag<double>;
using int_tag    = NativeVartypeTag<int>;
using struct_tag = NativeVartypeTag<FlexValue>;

using str_tag = StrLikeVartypeTag<char*, (-1)>;

//------------------------------------------------
// VartypeTag の中身を参照するインターフェース
//
// (「typename」地獄を避けるため)
//------------------------------------------------
// ベース
template<typename VartypeTag>
struct VtTraitsBase
	: public VartypeTag
{
	using value_t        = typename VartypeTag::value_t;
	using master_t       = typename VartypeTag::master_t;
	using valptr_t       = typename VartypeTag::valptr_t;
	using const_valptr_t = typename VartypeTag::const_valptr_t;

	static int const basesize = VartypeTag::basesize;

	// 固定長型か？
	static bool const isFixed_v = (VartypeTag::basesize >= 0);

	// 実体ポインタのキャスト
	static inline const_valptr_t asValptr(void const* pdat) {
		return reinterpret_cast<const_valptr_t>(pdat);
	}
	static inline valptr_t asValptr(void* pdat) { return const_cast<valptr_t>(asValptr(static_cast<void const*>(pdat))); }

	static inline PDAT const* asPDAT(const_valptr_t p) {
		return reinterpret_cast<PDAT const*>(p);
	}
	static inline PDAT* asPDAT(valptr_t p) { return const_cast<PDAT*>(asPDAT(static_cast<const_valptr_t>(p))); }

	// 実体ポインタの脱参照
	static inline value_t const& derefValptr(void const* pdat)
	{
		return *asValptr(pdat);
	}
	static inline value_t& derefValptr(void* pdat)
	{ return const_cast<value_t&>(derefValptr(static_cast<void const*>(pdat))); }

	static void derefValptr(PVal const* pval) { static_assert(false, "derefValptr(PVal*)...really?"); }

	// master ポインタのキャスト
	static inline master_t& getMaster(PVal* pval) {
		return *reinterpret_cast<master_t*>(&pval->master);
	}
};

// 実際に使う側
// 特殊化するのが楽になるように、実体を VtTraitsBase に出している
template<typename VartypeTag>
struct VtTraits
	: public VtTraitsBase<VartypeTag>
{ };

// 略記のための特殊化

template<> struct VtTraits<double>
	: public VtTraits<double_tag> { };
template<> struct VtTraits<int>
	: VtTraits<int_tag> { };

//------------------------------------------------
// 実体ポインタを得る
//
// @ pt が valptr_array である場合
//------------------------------------------------
template<typename VartypeTag>
static PDAT* HspVarTemplate_GetPtr(PVal* pval)
{
	assert(PVal_supportArray(pval));
	return reinterpret_cast<PDAT*>(VtTraits<VartypeTag>::asValptr(pval) + pval->offset);
}

//------------------------------------------------
// 可変長型に特有の関数の、固定長型の場合
//------------------------------------------------
template<typename VartypeTag>
static int HspVarTemplate_GetSize(PDAT const* pdat)
{
	static_assert(VtTraits<VartypeTag>::isFixed_v, "");
	return VtTraits<VartypeTag>::basesize;
}

template<typename VartypeTag>
static void* HspVarTemplate_GetBlockSize(PVal* pval, PDAT* pdat, int* size)
{
	static_assert(VtTraits<VartypeTag>::isFixed_v, "");
	*size = VtTraits<VartypeTag>::basesize * PVal_cntElems(pval);
	return pdat;
}

template<typename VartypeTag>
static void HspVarTemplate_AllocBlock(PVal* pval, PDAT* pdat, int size)
{
	static_assert(VtTraits<VartypeTag>::isFixed_v, "");
}

//------------------------------------------------
// 比較関数の具体化
//
// @ HspVar**_CmpI という1つの関数から演算関数を生成する。
// @ aftertype はその中で設定する必要がある。
//------------------------------------------------
using compare_func_t = int(*)(PDAT* pdat, void const* val);

namespace detail
{
template<compare_func_t CmpI, typename TCmpFunctor>
static void HspVarTemplate_CmpI(PDAT* pdat, void const* val)
{
	static TCmpFunctor comparer {};

	VtTraits<int>::derefValptr(pdat) = HspBool( comparer(CmpI(pdat, val), 0) );

//	myhvp->aftertype = HSPVAR_FLAG_INT;
	return;
}
}

// 同値性のみ定義する
template<compare_func_t CmpI>
static void HspVarTemplate_InitCmpI_Equality(HspVarProc* hvp)
{
	hvp->EqI = detail::HspVarTemplate_CmpI< CmpI, std::equal_to<int> >;
	hvp->NeI = detail::HspVarTemplate_CmpI< CmpI, std::not_equal_to<int> >;
	return;
}

// 比較関数をすべて定義する
template<compare_func_t CmpI>
static void HspVarTemplate_InitCmpI_Full(HspVarProc* hvp)
{
	HspVarTemplate_InitCmpI_Equality< CmpI >(hvp);

	hvp->LtI   = detail::HspVarTemplate_CmpI< CmpI, std::less<int> >;
	hvp->GtI   = detail::HspVarTemplate_CmpI< CmpI, std::greater<int> >;
	hvp->LtEqI = detail::HspVarTemplate_CmpI< CmpI, std::less_equal<int> >;
	hvp->GtEqI = detail::HspVarTemplate_CmpI< CmpI, std::greater_equal<int> >;
	return;
}


} // namespace hpimod

#endif
