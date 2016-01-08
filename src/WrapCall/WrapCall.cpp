#ifdef with_WrapCall

#include "../main.h"

#include "WrapCall.h"
#include "type_modcmd.h"
#include "ModcmdCallInfo.h"

//------------------------------------------------
// Knowbug 側へのコールバック
//------------------------------------------------
namespace Knowbug
{

extern void onBgnCalling(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo);
extern void onEndCalling(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT* ptr, vartype_t vtype);

} //namespace Knowbug

namespace WrapCall
{

static auto g_stkCallInfo = stkCallInfo_t {};

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
//
// ラップされたコマンド処理関数から呼ばれる。
//------------------------------------------------
void onBgnCalling(stdat_t stdat)
{
	g_dbginfo->updateCurInf();

	// 呼び出しリストに追加
	auto idx = g_stkCallInfo.size();
	g_stkCallInfo.emplace_back(new ModcmdCallInfo(
		stdat, ctx->prmstack, ctx->sublev, ctx->looplev,
		g_dbginfo->curFileName(), g_dbginfo->curLine(),
		idx
	));

	auto& callinfo = g_stkCallInfo.back();

	// DebugWindow への通知
	Knowbug::onBgnCalling(callinfo);
}

//------------------------------------------------
// 呼び出しの完了
//------------------------------------------------
void onEndCalling(PDAT* p, vartype_t vt)
{
	if (g_stkCallInfo.empty()) return;

	auto const& callinfo = g_stkCallInfo.back();

	// 警告
	if ( ctx->looplev != callinfo->looplev ) {
		Knowbug::logmesWarning("呼び出し中に入った loop から正常に脱出しないまま、呼び出しが終了した。");
	}

	if ( ctx->sublev != callinfo->sublev ) {
		Knowbug::logmesWarning("呼び出し中に入ったサブルーチンから正常に脱出しないまま、呼び出しが終了した。");
	}

	// DebugWindow への通知
	Knowbug::onEndCalling(callinfo, p, vt);

	g_stkCallInfo.pop_back();
}

void onEndCalling()
{
	return onEndCalling(nullptr, HSPVAR_FLAG_NONE);
}

//------------------------------------------------
// callinfo スタックへのアクセス
//------------------------------------------------
auto tryGetCallInfoAt(size_t idx) -> ModcmdCallInfo::shared_ptr_type
{
	return (0 <= idx && idx < g_stkCallInfo.size())
		? g_stkCallInfo.at(idx)
		: nullptr;
}

auto getCallInfoRange() -> stkCallInfoRange_t
{
	return make_pair_range(g_stkCallInfo);
}

} //namespace WrapCall

#endif //defined(with_WrapCall)
