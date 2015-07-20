#ifndef IG_CLASS_VARDATA_STRING_H
#define IG_CLASS_VARDATA_STRING_H

#include "main.h"
#include "DebugInfo.h"
#include "config_mng.h"

class CStructedStrWriter;
class CLineformedWriter;
class CTreeformedWriter;
class CStrBuf;
#if defined(with_Assoc) || defined(with_Vector) || defined(with_Array)
class CAssoc;
class CVector;
class CArray;
#endif
namespace Sysvar {
	enum Id;
}

// 変数データの文字列を作るクラス
class CVardataStrWriter
{
private:
	std::unique_ptr<CStructedStrWriter> writer_;

public:
	CVardataStrWriter(CVardataStrWriter&& src);
	~CVardataStrWriter();

	template<typename TWriter>
	static CVardataStrWriter create(std::shared_ptr<CStrBuf> buf)
	{
		return CVardataStrWriter(buf, static_cast<TWriter*>(nullptr));
	}

private:
	template<typename TWriter>
	CVardataStrWriter(std::shared_ptr<CStrBuf> buf,  TWriter* /* for template argument deduction */)
		: writer_(static_cast<CStructedStrWriter*>(new TWriter(buf, g_config->infiniteNest)))
	{ }

public:
	string const& getString() const;

	void addVar(char const* name, PVal const* pval);
	void addVarScalar(char const* name, PVal const* pval);
	void addVarScalar(char const* name, PVal const* pval, APTR aptr);
	void addVarArray(char const* name, PVal const* pval);
private:
	void addVarArrayRec(PVal const* pval, size_t const (&cntElem)[hpimod::ArrayDimMax + 1], size_t idxDim, APTR aptr_offset);
public:
	void addValue(char const* name, vartype_t type, PDAT const* ptr);
	void addValueString(char const* name, char const* str);
	void addValueStruct(char const* name, FlexValue const* fv);

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
	void addPrmstack(stdat_t stdat, std::pair<void const*, bool> prmstk);
	void addParameter(char const* name, stdat_t stdat, stprm_t stprm, void const* member, bool isSafe);

	void addSysvar(Sysvar::Id id);

#ifdef with_WrapCall
	void addCall(stdat_t stdat, std::pair<void const*, bool> prmstk);
	void addResult(stdat_t stdat, PDAT const* resultPtr, vartype_t type);
#endif

public:
	CStructedStrWriter& getWriter() const { return *writer_; }
};

#endif
