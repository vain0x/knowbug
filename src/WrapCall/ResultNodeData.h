#ifndef IG_RESULT_NODE_DATA_H
#define IG_RESULT_NODE_DATA_H

// todo: WrapCall 本体とはあまり関係がないので適切な位置に移動させる

#include <memory>

#include "../main.h"
#include "../config_mng.h"
#include "../CVardataString.h"
#include "WrapCall/ModcmdCallInfo.h"

namespace WrapCall
{
	// Remark: Don't rearrange the members.
	struct ResultNodeData
	{
		stdat_t stdat;

		// 返値の型
		vartype_t vtype;

		// 値の文字列化
		string valueString;

		// これに依存する呼び出し (存在する場合はこれの子ノードになる)
		optional_ref<ModcmdCallInfo const> pCallInfoDepended;

	public:
		ResultNodeData(ModcmdCallInfo const& callinfo, PDAT* ptr, vartype_t vt);
		ResultNodeData(ModcmdCallInfo const& callinfo, PVal* pvResult)
			: ResultNodeData(callinfo, pvResult->pt, pvResult->flag)
		{ }
	};

} // namespace WrapCall

#endif
