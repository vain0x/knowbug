#ifndef IG_VARTREE_NODE_DATA_H
#define IG_VARTREE_NODE_DATA_H

#include <memory>

#include "main.h"
#include "config_mng.h"
#include "WrapCall/ModcmdCallInfo.h"

#ifdef with_WrapCall

struct ResultNodeData
{
	WrapCall::ModcmdCallInfo::shared_ptr_type const callinfo;

	// 返値の型
	vartype_t const vtype;

	// 値の文字列化
	string const treeformedString;
	string const lineformedString;

	// これに依存する呼び出し
	WrapCall::ModcmdCallInfo::shared_ptr_type const pCallInfoDepended;

public:
	ResultNodeData(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT const* ptr, vartype_t vt);
	ResultNodeData(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, PVal const* pvResult);
};

#endif //defined(with_WrapCall)

#endif
