// todo: WrapCall 本体とはあまり関係がないので適切な位置に移動させる

#pragma once

#include <memory>

#include "../main.h"
#include "../CVarinfoLine.h"
#include "../config_mng.h"
#include "WrapCall/ModcmdCallInfo.h"

namespace WrapCall
{
	// Remark: Don't rearrange the members.
	struct ResultNodeData
	{
		stdat_t stdat;
		int sublev;

		// 値の文字列化 (double, str, or int)
		string valueString;

		// これに依存する呼び出し (存在する場合はこれの子ノードになる)
		ModcmdCallInfo const* pCallInfoDepended;

	public:
		// ctor
		ResultNodeData(ModcmdCallInfo const& callinfo, void* ptr, vartype_t vt)
			: stdat(callinfo.stdat)
			, sublev(callinfo.sublev)
			, pCallInfoDepended(callinfo.getPrev())
		{
			auto const varinf = std::make_unique<CVarinfoLine>(g_config->maxlenVarinfo);
			varinf->addResult(ptr, vt);
			valueString = std::move(varinf->getString());
			return;
		}

		ResultNodeData(ModcmdCallInfo const& callinfo, PVal* pvResult)
			: ResultNodeData(callinfo, pvResult->pt, pvResult->flag)
		{ }
	};

} // namespace WrapCall
