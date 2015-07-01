// 変数データのツリー形式文字列

#ifndef IG_CLASS_VARINFO_TREE_H
#define IG_CLASS_VARINFO_TREE_H

#include "main.h"
#include "DebugInfo.h"
#include "SysvarData.h"
#include "module/strf.h"
#include "module/CStrBuf.h"

#if defined(with_Assoc) || defined(with_Vector) || defined(with_Array)
class CAssoc;
class CVector;
class CArray;
#endif

class CVarinfoTree
{
	// メンバ変数
private:
	CTreeformStrBuf mBuf;

	// メンバ関数
public:
	explicit CVarinfoTree(int lenLimit = (0x7FFFFFFF - 1));

	string const& getString() const { return mBuf.get(); }

	void addVar(char const* name, PVal const* pval);
	void addVarScalar(char const* name, PVal const* pval);
	void addVarScalar(char const* name, PVal const* pval, APTR aptr);
	void addVarArray(char const* name, PVal const* pval);

	void addValue(char const* name, vartype_t type, void const* ptr);
	void addItem_flexValue(char const* name, FlexValue const* fv);

#ifdef with_Assoc
	void addItem_assoc(char const* name, CAssoc* src);
#endif
#ifdef with_Vector
	void addItem_vector(char const* name, CVector* src);
#endif
#ifdef with_Array
	void addItem_array(char const* name, CArray* src);
#endif
#ifdef with_ExtraBasics
	//	template<class TNumeric> string dbgstr_extraBasic(const TNumeric src);
#endif
	//	void addItem_string(char const* src);

	void addPrmstack(stdat_t stdat, void const* prmstack);
	void addParameter(char const* name, stdat_t stdat, stprm_t stprm, void const* member);

	void addSysvar(SysvarId id);

	void addCall(stdat_t stdat, void const* prmstk);
	void addCall(char const* name, stdat_t stdat, void const* prmstk);
	void addResult(char const* name, void const* ptr, vartype_t type);

private:
	CVarinfoTree(CVarinfoTree const&) = delete;
};

// 単純な値を文字列化する
extern string stringizeSimpleValue( vartype_t type, void const* ptr, bool bShort );

// 文字列リテラルの形式の文字列
extern string toStringLiteralFormat( char const* src );

// モジュールクラス名の文字列の生成
extern string makeModuleClassNameString(stdat_t stdat, bool bClone);

// 配列添字の文字列の生成
extern string makeArrayIndexString(size_t dim, int const indexes[]);

// aptr から添字を求める
extern void calcIndexesFromAptr(int* indexes, APTR aptr, int const* lengths, size_t cntElems, size_t dim);

// const 変数の値を一時的に変更する
#if 0
#include <functional>
template<typename T, typename TVal, typename TResult,
	typename = std::enable_if_t<!std::is_same<TResult, void>::value>
> TResult setASideConstTemporarily(T const& v, TVal&& tempVal, std::function<TResult()> const& proc)
{
	static_assert(std::is_convertible<TVal&&, T const&>::value, "");
	
	T const bak { std::move(v) };
	const_cast<T&>(v) = std::forward(tempVal);
	auto&& result { proc() };
	const_cast<T&>(v) = std::move(bak);
	return std::move(result);
}

template<typename T, typename TVal>
void setASideConstTemporarily(T const& v, TVal&& tempVal, std::function<void()> const& proc)
{
	T const bak { std::move(v) };
	const_cast<T&>(v) = std::forward(tempVal);
	proc();
	const_cast<T&>(v) = std::move(bak);
	return;
}
#endif

#if 0
template<typename T>
class TmpConstValueChange
{
public:
	TmpConstValueChange(T const& ref, T const& tempVal)
		: ref_(ref)
		, bak_(std::move(ref))
	{
		const_cast<T&>(ref_) = std::move(tempVal);
	}
	TmpConstValueChange(TmpConstValueChange const&) = delete;
	TmpConstValueChange& operator =(TmpConstValueChange const&) = delete;
	~TmpConstValueChange() { const_cast<T&>(ref_) = std::move(bak_); }
private:
	T const& ref_;
	T const bak_;
};
#endif

#endif
