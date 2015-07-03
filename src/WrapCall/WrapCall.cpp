// WrapCall

#include <vector>

#include "../main.h"
#include "../DebugInfo.h"

#include "WrapCall.h"
#include "ModcmdCallInfo.h"
#include "type_modcmd.h"

namespace WrapCall
{

static stkCallInfo_t g_stkCallInfo;

//------------------------------------------------
// プラグイン初期化関数
//------------------------------------------------
EXPORT void WINAPI hsp3hpi_init_wrapcall(HSP3TYPEINFO* info)
{
	hsp3sdk_init(info);

	// 初期化
	auto const typeinfo = &info[- info->type];
	modcmd_init(&typeinfo[TYPE_MODCMD]);
	g_stkCallInfo.reserve(32);
}

//------------------------------------------------
// 呼び出しの開始
//------------------------------------------------
void bgnCall(stdat_t stdat)
{
	g_dbginfo->updateCurInf();

	// 呼び出しリストに追加
	size_t const idx = g_stkCallInfo.size();
	g_stkCallInfo.push_back(std::make_unique<ModcmdCallInfo>(
		stdat, ctx->prmstack, ctx->sublev, ctx->looplev,
		g_dbginfo->curFileName(), g_dbginfo->curLine(), idx
	));

	auto& callinfo = *g_stkCallInfo.back();

	// DebugWindow への通知
	Knowbug::bgnCalling(callinfo);
}

//------------------------------------------------
// 呼び出しの完了
//------------------------------------------------
void endCall()
{
	return endCall(nullptr, HSPVAR_FLAG_NONE);
}

void endCall(PDAT* p, vartype_t vt)
{
	if (g_stkCallInfo.empty()) return;

	auto const& callinfo = *g_stkCallInfo.back();

	// 警告
	if ( ctx->looplev != callinfo.looplev ) {
		Knowbug::logmesWarning("呼び出し中に入った loop から正常に脱出しないまま、呼び出しが終了した。");
	}

	if ( ctx->sublev != callinfo.sublev ) {
		Knowbug::logmesWarning("呼び出し中に入ったサブルーチンから正常に脱出しないまま、呼び出しが終了した。");
	}

	// DebugWindow への通知
	Knowbug::endCalling(callinfo, p, vt);

	g_stkCallInfo.pop_back();
}

//------------------------------------------------
// callinfo スタックへのアクセス
//------------------------------------------------
ModcmdCallInfo const* getCallInfoAt(size_t idx)
{
	return ( 0 <= idx && idx < g_stkCallInfo.size() )
		? g_stkCallInfo.at(idx).get()
		: nullptr;
}

stkCallInfoRange_t getCallInfoRange()
{
	return make_pair_range(g_stkCallInfo);
}

}
