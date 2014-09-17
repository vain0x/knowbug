#ifndef IG_VARTYPE_TRAITS_H
#define IG_VARTYPE_TRAITS_H

#include "hsp3plugin_custom.h"

namespace hpimod
{

//------------------------------------------------
// traits
//------------------------------------------------

// pt = TValue[] であるような素朴な型の tag
template<typename TValue>
struct NativeVartypeTag
{
	using value_t = TValue;
	using valptr_t = TValue*;
	using const_valptr_t = TValue const*;
	using master_t = void*;

	static int const basesize = sizeof(TValue);
	static vartype_t vartype();

	// 変数から実体ポインタを得る
	static const_valptr_t& getValptr(PVal const* pval) {
		assert(pval->flag == vartype());
		return &reinterpret_cast<const_valptr_t>(pval->pt)[pval->offset];
	}
	static valptr_t& getValptr(PVal* pval) { return const_cast<valptr_t&>(getValptr(static_cast<PVal const*>(pval))); }
};

// str 型のような型の tag
// master = TValue[], pt = master[0] となる。
template<typename TValue, typename TConstValue, int TBasesize>
struct StrLikeVartypeTag
{
	using value_t = TValue;
	using valptr_t = TValue;
	using const_valptr_t = TConstValue;

	using master_t = TValue*;

	static int const basesize = TBasesize;
};

// 組み込み型の変数型タグ
namespace detail
{
	// 型タイプ値は定数式で与えられる
	template<vartype_t NVartype, typename TagSuper>
	struct InternalVartypeTag : public TagSuper {
		static vartype_t vartype() { return NVartype; }
	};
}

using label_tag  = detail::InternalVartypeTag< HSPVAR_FLAG_LABEL,  NativeVartypeTag<label_t> >;
using double_tag = detail::InternalVartypeTag< HSPVAR_FLAG_DOUBLE, NativeVartypeTag<double> >;
using int_tag    = detail::InternalVartypeTag< HSPVAR_FLAG_INT,    NativeVartypeTag<int> >;
using struct_tag = detail::InternalVartypeTag< HSPVAR_FLAG_STRUCT, NativeVartypeTag<FlexValue> >;

using str_tag    = detail::InternalVartypeTag< HSPVAR_FLAG_STR, StrLikeVartypeTag<char*, char const*, (-1)> >;

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
	using value_t = typename VartypeTag::value_t;
	using valptr_t = typename VartypeTag::valptr_t;
	using const_valptr_t = typename VartypeTag::const_valptr_t;
	using master_t = typename VartypeTag::master_t;

	static int const basesize = VartypeTag::basesize;

	// 固定長型か？
	static bool const isFixed_v = (VartypeTag::basesize >= 0);

	// 実体ポインタと PDAT* の相互キャスト
	static inline const_valptr_t asValptr(PDAT const* pdat) {
		return reinterpret_cast<const_valptr_t>(pdat);
	}
	static inline valptr_t asValptr(PDAT* pdat) { return const_cast<valptr_t>(asValptr(static_cast<PDAT const*>(pdat))); }

	static inline PDAT const* asPDAT(const_valptr_t p) {
		return reinterpret_cast<PDAT const*>(p);
	}
	static inline PDAT* asPDAT(valptr_t p) { return const_cast<PDAT*>(asPDAT(static_cast<const_valptr_t>(p))); }

	// 実体ポインタの脱参照
	static inline value_t const& derefValptr(PDAT const* pdat)
	{
		static_assert(std::is_same<value_t*, valptr_t>::value, "既定の derefValptr は valptr_t = value_t* となる変数型にのみ使用できる。");
		return *asValptr(pdat);
	}
	static inline value_t& derefValptr(PDAT* pdat)
	{ return const_cast<value_t&>(derefValptr(static_cast<PDAT const*>(pdat))); }

	// PVal::master のキャスト
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

template<> struct VtTraits<label_t> : public VtTraits<label_tag> {};
template<> struct VtTraits<double>  : public VtTraits<double_tag> {};
template<> struct VtTraits<int>     : public VtTraits<int_tag> {};

} // namespace hpimod

#endif
