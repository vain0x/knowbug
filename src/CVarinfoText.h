// 変数データテキスト生成クラス

#ifndef IG_CLASS_VARINFO_TEXT_H
#define IG_CLASS_VARINFO_TEXT_H

#include "main.h"
#include "DebugInfo.h"
#include "module/strf.h"
#include "module/CStrBuf.h"

#ifdef with_WrapCall
namespace WrapCall
{
	struct ModcmdCallInfo;
}
#endif

class CVarinfoText
{
	// メンバ変数
private:
	CStrBuf mBuf;

	// メンバ関数
public:
	explicit CVarinfoText(int lenLimit = (0x7FFFFFFF - 1));
	~CVarinfoText() = default;

	string const& getString() const { return mBuf.get(); }

	void addVar(PVal* pval, char const* name);
	void addSysvar(char const* name);
#ifdef with_WrapCall
	void addCall(WrapCall::ModcmdCallInfo const& callinfo);
	void addResult(stdat_t stdat, void* ptr, vartype_t vtype, char const* name);
	void addResult2(string const& text, char const* name);
#endif

	// 項目の追加
	//	void addItem( char const* name, char const* string );
	//	void addItem( char const* name, char const* format, ... );

	// 文字列連結
	void catln(char const* s);
};

char const* getMPTypeString(int mptype);
string getVartypeString(PVal const* pval);

#endif
