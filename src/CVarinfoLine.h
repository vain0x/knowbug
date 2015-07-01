// 変数データの一行文字列

#ifndef IG_CLASS_VARINFO_LINE_H
#define IG_CLASS_VARINFO_LINE_H

#include "DebugInfo.h"
#include "SysvarData.h"
#include "module/strf.h"
#include "module/CStrBuf.h"

#if defined(with_Assoc) || defined(with_Vector) || defined(with_Array)
class CAssoc;
class CVector;
class CArray;
#endif
#ifdef with_WrapCall
namespace WrapCall
{
	struct ResultNodeData;
} // namespace WrapCall
#endif

class CVarinfoLine
{
	// メンバ変数
private:
	CStrBuf mBuf;

	// メンバ関数
public:
	explicit CVarinfoLine(int lenLimit = (0x7FFFFFFF - 1));

	string const& getString() const { return mBuf.get(); }

	void addVar(PVal const* pval);
	void addVarScalar(PVal const* pval);
	void addVarScalar(PVal const* pval, APTR aptr);
	void addVarArray(PVal const* pval);
	void addVarArrayRec(PVal const* pval, size_t const cntElem[], size_t idxDim, APTR aptr_offset);
	
	void addValue(vartype_t type, void const* ptr);
	void addItem_flexValue(FlexValue const* fv);

#ifdef with_Assoc
	void addItem_assoc(CAssoc* src);
#endif
#ifdef with_Vector
	void addItem_vector(CVector* src);
#endif
#ifdef with_Array
	void addItem_array    ( CArray* src );
#endif
#ifdef with_ExtraBasics
	//	template<class TNumeric> string dbgstr_extraBasic( BaseData const& base, const TNumeric src );
#endif
	//	void addItem_string   ( BaseData const& base, char const* src );

	void addPrmstack(stdat_t stdat, void const* prmstack);
	void addParameter(stdat_t stdat, stprm_t stprm, void const* member);

	void addSysvar(SysvarId id);
	void addCall(stdat_t stdat, void const* prmstk);
	void addResult(void const* ptr, vartype_t type);

	void addModuleOverview(char const* name, CVarTree::ModuleNode const& tree);
	void addSysvarsOverview();
#ifdef with_WrapCall
	void addCallsOverview(WrapCall::ResultNodeData const* pLastResult);
#endif
	
	// 文字列の連結
	/*
public:
	void cat(string const& src) { cat(src.c_str()); }
	void cat(char const* src, size_t len);
	void cat(char const* src);
	void cat_crlf();
	//*/
public:
	void cat(char const* s) { mBuf.cat(s); }
	void cat(string const& s) { cat(s.c_str()); }
	void catln(char const* s) { mBuf.catln(s); }
	void catln(string const& s) { mBuf.catln(s.c_str()); }
	void catCrlf() { mBuf.catCrlf(); }

private:
	CVarinfoLine(CVarinfoLine const&) = delete;

};

// 以下定義は CVarinfoTree.cpp

extern string stringizeSimpleValue(vartype_t type, void const* ptr, bool bShort);
extern string makeModuleClassNameString(stdat_t stdat, bool bClone);
extern string makeArrayIndexString(size_t dim, int const indexes[]);

#endif
