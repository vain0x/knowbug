#ifndef IG_VARTYPE_TRAITS_H
#define IG_VARTYPE_TRAITS_H

#include "hsp3plugin_custom.h"

namespace hpimod
{

//------------------------------------------------
// 変数型の特性
//------------------------------------------------
namespace VtTraits
{
	namespace Impl
	{
		//------------------------------------------------
		// 実体型
		//------------------------------------------------
		template<typename Tag> struct value_type;
		template<typename Tag> struct const_value_type;
		//{ using type = value_type<Tag>::type const; };

		//------------------------------------------------
		// 実体ポインタ型 (PDAT* と互換)
		//------------------------------------------------
		template<typename Tag> struct valptr_type;
		template<typename Tag> struct const_valptr_type;

		//------------------------------------------------
		// マスター型 (PVal::master の型と互換)
		//------------------------------------------------
		template<typename Tag> struct master_type {
			using type = void*;
		};

		//------------------------------------------------
		// ベースサイズ
		//------------------------------------------------
		template<typename Tag> struct basesize;

		//------------------------------------------------
		// 型タイプ値 (を返す関数)
		//------------------------------------------------
		template<typename Tag> struct vartype;
	}

	//------------------------------------------------
	// alias
	//------------------------------------------------
	template<typename Tag> using value_t        = typename Impl::value_type<Tag>::type;
	template<typename Tag> using const_value_t  = typename Impl::const_value_type<Tag>::type;
	template<typename Tag> using valptr_t       = typename Impl::valptr_type<Tag>::type;
	template<typename Tag> using const_valptr_t = typename Impl::const_valptr_type<Tag>::type;
	template<typename Tag> using master_t       = typename Impl::master_type<Tag>::type;

	template<typename Tag> using basesize = Impl::basesize<Tag>;	// 変数テンプレートがないので ::value は外せない
	template<typename Tag> using vartype = Impl::vartype<Tag>;

	//------------------------------------------------
	// value[] の型、のインターフェース的なもの
	//
	// この方法では、タグは高々1つのインターフェースしか持てない。
	// 一応対策はあるがメタプログラミングライブラリが必要になるのでめんどくさい。
	// using Int = VartypeTag<NativeVartypeTag<Int>, InternalTag<Int>>; としておいて、
	// 部分特殊化は VartypeTag<Attrs...> で受け、対応する属性が Attrs... の中にあるかどうかについて SFINAE で場合分け。
	// またこれのせいで int 型のタグを int にするということもできない。
	//------------------------------------------------
	template<typename TValue>
	struct NativeVartypeTag { };

	namespace Impl
	{
		template<typename T> struct value_type<NativeVartypeTag<T>> {
			using type = T;
		};
		template<typename T> struct const_value_type<NativeVartypeTag<T>> {
			using type = T const;
		};
		template<typename T> struct valptr_type<NativeVartypeTag<T>> {
			using type = T*;
		};
		template<typename T> struct const_valptr_type<NativeVartypeTag<T>> {
			using type = T const*;
		};
		template<typename T> struct basesize<NativeVartypeTag<T>> {
			static int const value = sizeof(T);
		};
	}

	//------------------------------------------------
	// 型に関する各種関数
	//------------------------------------------------

	// 固定長型か？
	template<typename Tag>
	struct isFixed { static bool const value = (basesize<Tag>::value >= 0); };

	// PDAT* → 実体ポインタ
	template<typename Tag>
	static const_valptr_t<Tag> asValptr(PDAT const* pdat) {
		return reinterpret_cast<const_valptr_t<Tag>>(pdat);
	}
	template<typename Tag>
	static valptr_t<Tag> asValptr(PDAT* pdat) { return const_cast<valptr_t<Tag>>(asValptr<Tag>(static_cast<PDAT const*>(pdat))); }

	// 実体ポインタ → PDAT*
	template<typename Tag>
	static PDAT const* asPDAT(const_valptr_t<Tag> p) {
		return reinterpret_cast<PDAT const*>(p);
	}
	template<typename Tag>
	static PDAT* asPDAT(valptr_t<Tag> p) { return const_cast<PDAT*>(asPDAT<Tag>(static_cast<const_valptr_t<Tag>>(p))); }

	// PVal::master のキャスト
	// todo: const_master_t も必要？
	template<typename Tag>
	static master_t<Tag> const& getMaster(PVal const* pval) {
		return *reinterpret_cast<master_t<Tag> const*>(&pval->master);
	}
	template<typename Tag>
	static master_t<Tag>& getMaster(PVal* pval) { return const_cast<master_t<Tag>&>(getMaster<Tag>(static_cast<PVal const*>(pval))); }

	// 実体ポインタの脱参照 (valptr_t = value_t* である型に限る)
	template<typename Tag> static const_value_t<Tag>& derefValptr(PDAT const* pdat) {
		static_assert(std::is_same<valptr_t<Tag>, value_t<Tag>*>::value, "General 'derefValptr()' can be used for vartypes (valptr_t = value_t*).");
		return *asValptr<Tag>(pdat);
	}
	template<typename Tag>
	static value_t<Tag>& derefValptr(PDAT* pdat) { return const_cast<value_t<Tag>&>(derefValptr<Tag>(static_cast<PDAT const*>(pdat))); }

	//------------------------------------------------
	// 組み込み型のタグ
	//------------------------------------------------
	namespace InternalVartypeTags
	{
		using vtLabel  = NativeVartypeTag<label_t>;
		using vtDouble = NativeVartypeTag<double>;
		using vtInt    = NativeVartypeTag<int>;
		using vtStruct = NativeVartypeTag<FlexValue>;

		struct vtStr { };
	}

	// 型タイプ値
	namespace Impl
	{
		using namespace InternalVartypeTags;

		template<> struct vartype<vtLabel>  { static vartype_t apply() { return HSPVAR_FLAG_LABEL; } };
		template<> struct vartype<vtStr>    { static vartype_t apply() { return HSPVAR_FLAG_STR; } };
		template<> struct vartype<vtDouble> { static vartype_t apply() { return HSPVAR_FLAG_DOUBLE; } };
		template<> struct vartype<vtInt>    { static vartype_t apply() { return HSPVAR_FLAG_INT; } };
		template<> struct vartype<vtStruct> { static vartype_t apply() { return HSPVAR_FLAG_STRUCT; } };
	}

	// str 型の特性の定義
	namespace Impl
	{
		template<> struct value_type<vtStr>  { using type = char*; };
		template<> struct const_value_type<vtStr>  { using type = char const*; };
		template<> struct valptr_type<vtStr> { using type = char*; };
		template<> struct const_valptr_type<vtStr> { using type = char const*; };
		template<> struct master_type<vtStr> { using type = char**; };
		template<> struct basesize<vtStr> { static int const value = -1; };
	}

	// str 型のオーバーライド

	// 実体ポインタの脱参照: deref は左辺値を返すが、実体ポインタから左辺値を得られないので定義できない。
	//template<> static inline const_value_t<vtStr>& derefValptr<vtStr>(PDAT const* pdat) { };
} // namespace VtTraits

//------------------------------------------------
// NativeVartype 専用の関数
//------------------------------------------------
namespace VtTraits
{
	// NativeVartypeTag かどうか
	template<typename Tag> struct isNativeVartype { static bool const value = false; };
	template<typename TVal> struct isNativeVartype<NativeVartypeTag<TVal>> { static bool const value = true; };

	// 変数から実体ポインタを得る
	template<typename Tag> static const_valptr_t<Tag> getValptr(PVal const* pval) {
		static_assert(isNativeVartype<Tag>::value, "'getValptr' for non-NativeVartype types is undefined.");
		assert(pval->flag == vartype<Tag>::apply());
		return asValptr<Tag>(pval->pt) + pval->offset;
	}
	template<typename Tag>
	static valptr_t<Tag> getValptr(PVal* pval) { return const_cast<valptr_t<Tag>>(getValptr<Tag>(static_cast<PVal const*>(pval))); }
} // namespace VtTraits

using namespace VtTraits::InternalVartypeTags;

} // namespace hpimod

#endif
