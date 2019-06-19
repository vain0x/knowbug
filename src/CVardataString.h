#ifndef IG_CLASS_VARDATA_STRING_H
#define IG_CLASS_VARDATA_STRING_H

#include "main.h"
#include "DebugInfo.h"
#include "config_mng.h"

namespace hpiutil {
	class DInfo;
}

class CStructedStrWriter;
class CLineformedWriter;
class CTreeformedWriter;
class CStrBuf;
class HspStaticVars;

// 変数データの文字列を作るクラス
class CVardataStrWriter
{
private:
	std::unique_ptr<CStructedStrWriter> writer_;
	mutable unordered_map<void const*, string> visited_;

	hpiutil::DInfo const& debug_segment_;
	HspStaticVars& static_vars_;

public:
	CVardataStrWriter(CVardataStrWriter&& src);
	~CVardataStrWriter();

	CVardataStrWriter(std::unique_ptr<CStructedStrWriter>&& writer, hpiutil::DInfo const& debug_segment, HspStaticVars& static_vars)
		: writer_(std::move(writer))
		, debug_segment_(debug_segment)
		, static_vars_(static_vars)
	{
	}

public:
	auto getString() const -> string const&;

	void addVar(char const* name, PVal const* pval);
	void addVarScalar(char const* name, PVal const* pval);
	void addVarScalar(char const* name, PVal const* pval, APTR aptr);
	void addVarArray(char const* name, PVal const* pval);
private:
	using indexes_t = std::array<size_t, 1 + hpiutil::ArrayDimMax>;
	void addVarArrayRec(PVal const* pval, indexes_t const& cntElem, size_t idxDim, APTR aptr_offset);
public:
	void addValue(char const* name, vartype_t type, PDAT const* ptr);
	void addValueString(char const* name, char const* str);
	void addValueStruct(char const* name, FlexValue const* fv);

	void addPrmstack(stdat_t stdat, std::pair<void const*, bool> prmstk);
	void addParameter(char const* name, stdat_t stdat, stprm_t stprm, void const* member, bool isSafe);

	void addSysvar(hpiutil::Sysvar::Id id);

#ifdef with_WrapCall
	void addCall(stdat_t stdat, std::pair<void const*, bool> prmstk);
#endif

public:
	CStructedStrWriter& getWriter() const { return *writer_; }

private:
	bool tryPrune(char const* name, void const* ptr) const;
};

#endif
