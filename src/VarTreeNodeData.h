#ifndef IG_VARTREE_NODE_DATA_H
#define IG_VARTREE_NODE_DATA_H

#include <memory>

#include "main.h"
#include "config_mng.h"
#include "WrapCall/ModcmdCallInfo.h"

struct ResultNodeData
{
	stdat_t stdat;

	// 返値の型
	vartype_t vtype;

	// 値の文字列化
	string valueString;

	// これに依存する呼び出し (存在する場合はこれの子ノードになる)
	optional_ref<WrapCall::ModcmdCallInfo const> pCallInfoDepended;

public:
	ResultNodeData(WrapCall::ModcmdCallInfo const& callinfo, PDAT* ptr, vartype_t vt);
	ResultNodeData(WrapCall::ModcmdCallInfo const& callinfo, PVal* pvResult)
		: ResultNodeData(callinfo, pvResult->pt, pvResult->flag)
	{ }
};

#endif
