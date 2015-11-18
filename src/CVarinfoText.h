#ifndef IG_CLASS_VARINFO_TEXT_H
#define IG_CLASS_VARINFO_TEXT_H

#include "main.h"
#include "module/CStrWriter.h"

class VTNodeModule;
struct ResultNodeData;

namespace WrapCall
{
	struct ModcmdCallInfo;
}

// 変数データテキスト生成クラス
class CVarinfoText
{
public:
	CVarinfoText();

	void addVar(PVal* pval, char const* name);
	void addSysvar(hpiutil::Sysvar::Id id);
#ifdef with_WrapCall
	void addCall(WrapCall::ModcmdCallInfo const& callinfo);
	void addResult(ResultNodeData const& result);
private:
	void addCallSignature(WrapCall::ModcmdCallInfo const& callinfo, stdat_t stdat);
public:
#endif

	void addModuleOverview(char const* name, VTNodeModule const& tree);
	void addSysvarsOverview();
#ifdef with_WrapCall
	void addCallsOverview(optional_ref<ResultNodeData const> lastResult);
#endif
	void addGeneralOverview();

	string const& getString() const;
	string&& getStringMove();
private:
	CStrWriter& getWriter() { return writer_; }
	std::shared_ptr<CStrBuf> getBuf() const { return writer_.getBuf(); }
private:
	CStrWriter writer_;
};

extern string stringizePrmlist(stdat_t stdat);
extern string stringizeVartype(PVal const* pval);

#endif
