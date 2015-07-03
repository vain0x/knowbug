#ifndef IG_CLASS_VARINFO_TEXT_H
#define IG_CLASS_VARINFO_TEXT_H

#include "main.h"
#include "module/CStrWriter.h"

class StaticVarTree;
struct ResultNodeData;

namespace WrapCall
{
	struct ModcmdCallInfo;
}

namespace Sysvar
{
	enum Id;
}

// 変数データテキスト生成クラス
class CVarinfoText
{
public:
	CVarinfoText();

	void addVar(PVal* pval, char const* name);
	void addSysvar(Sysvar::Id id);
#ifdef with_WrapCall
	void addCall(std::shared_ptr<WrapCall::ModcmdCallInfo const> const& callinfo);
	void addResult(stdat_t stdat, string const& text, char const* name);
#endif

	void addModuleOverview(char const* name, StaticVarTree const& tree);
	void addSysvarsOverview();
#ifdef with_WrapCall
	void addCallsOverview(ResultNodeData const* pLastResult);
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
