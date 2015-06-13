#ifndef IG_CLASS_VARINFO_TEXT_H
#define IG_CLASS_VARINFO_TEXT_H

#include "main.h"
#include "DebugInfo.h"
#include "module/CStrWriter.h"

class CStaticVarTree;

#ifdef with_WrapCall
namespace WrapCall
{
	struct ModcmdCallInfo;
	struct ResultNodeData;
}
#endif
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
	void addCall(WrapCall::ModcmdCallInfo const& callinfo);
	void addResult(stdat_t stdat, string const& text, char const* name);
#endif
	void addCustom(label_t lb);

	void addModuleOverview(char const* name, CStaticVarTree const& tree);
	void addSysvarsOverview();
#ifdef with_WrapCall
	void addCallsOverview(WrapCall::ResultNodeData const* pLastResult);
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

extern char const* getMPTypeString(int mptype);
extern string getPrmlistString(stdat_t stdat);
extern string getVartypeString(PVal const* pval);

#endif
