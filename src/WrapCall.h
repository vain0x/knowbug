
#pragma once

#ifdef with_WrapCall

#include <vector>
# include "../../../../../../MakeHPI/WrapCall/ModcmdCallInfo.h"
# include "../../../../../../MakeHPI/WrapCall/DbgWndMsg.h"
# include "../../../../../../MakeHPI/WrapCall/WrapCallSdk.h"

struct ResultNodeData {
	STRUCTDAT* stdat;
	int sublev;
	string valueString;						// 値の文字列化 (double, str, or int)
	const ModcmdCallInfo* pCallInfoDepended;	// これに依存する呼び出し (存在する場合はこれの子ノードになる)
};

extern std::vector<const ModcmdCallInfo*> g_stkCallInfo;

void* ModcmdCallInfo_getPrmstk(const ModcmdCallInfo& callinfo);

void WrapCall_RequireMethodFunc( WrapCallMethod* methods );

#endif
