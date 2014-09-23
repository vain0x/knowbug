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

static int g_pluginType_WrapCall = -1;

/*
struct ValueRef
{
	PDAT*      p;
	vartype_t vt;
public:
	ValueRef()
		: p(nullptr)
		, vt(HSPVAR_FLAG_NONE)
	{ }

	void set(PDAT* p1, vartype_t vt1) {
		p = p1; vt = vt1;
	}
};
static ValueRef lastResult;	// 最後の返値への参照 (すぐ使えなくなるので注意)
//*/

//------------------------------------------------
// プラグイン初期化関数
//------------------------------------------------
EXPORT void WINAPI hsp3hpi_init_wrapcall(HSP3TYPEINFO* info)
{
	g_pluginType_WrapCall = info->type;
	auto const typeinfo = info - g_pluginType_WrapCall;

	hsp3sdk_init(info);

	// 初期化
	modcmd_init(&typeinfo[TYPE_MODCMD]);
	g_stkCallInfo.reserve(32);
	return;
}

/*
EXPORT void WINAPI WrapCallInitialize()
{
	//
}

// typeinfo が動いた可能性があるので元に戻すタイミングはない
EXPORT void WINAPI WrapCallTerminate()
{
	if ( g_typeinfo != nullptr ) {
		modcmd_term(&g_typeinfo[TYPE_MODCMD]);
	}
	return;
}
//*/

//------------------------------------------------
// 呼び出しの開始
//------------------------------------------------
void bgnCall(stdat_t stdat)
{
	g_dbginfo->debug->dbg_curinf();

	// 呼び出しリストに追加
	size_t const idx = g_stkCallInfo.size();
	g_stkCallInfo.push_back(std::make_unique<ModcmdCallInfo>(
		stdat, ctx->prmstack, ctx->sublev, ctx->looplev,
		g_dbginfo->debug->fname, g_dbginfo->debug->line, idx
	));

	auto& callinfo = *g_stkCallInfo.back();
	
	// DebugWindow への通知
	Knowbug::bgnCalling(callinfo);
	return;
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
		Knowbug::logmesWarning( "呼び出し中に入った loop から、正常に脱出せず、呼び出しが終了した。" );
	}
	
	if ( ctx->sublev != callinfo.sublev ) {
		Knowbug::logmesWarning("呼び出し中に入ったサブルーチンから、正常に脱出せず、呼び出しが終了した。");
	}

	// DebugWindow への通知
	Knowbug::endCalling(callinfo, p, vt);
	
	g_stkCallInfo.pop_back();
	return;
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

std::pair<stkCallInfo_t::const_iterator, stkCallInfo_t::const_iterator> getCallInfoRange()
{
	return std::make_pair(g_stkCallInfo.begin(), g_stkCallInfo.end());
}

}
