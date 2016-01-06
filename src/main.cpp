
#include <winapifamily.h>
#include "main.h"
#include "module/strf.h"
#include "DebugInfo.h"
#include "VarTreeNodeData.h"
#include "CVarinfoText.h"

#include "config_mng.h"
#include "dialog.h"

static HINSTANCE g_hInstance;
std::unique_ptr<DebugInfo> g_dbginfo;

// ランタイムとの通信
EXPORT BOOL WINAPI debugini( HSP3DEBUG* p1, int p2, int p3, int p4 );
EXPORT BOOL WINAPI debug_notice( HSP3DEBUG* p1, int p2, int p3, int p4 );
static void debugbye();

// WrapCall 関連
#include "WrapCall/WrapCall.h"
#include "WrapCall/ModcmdCallInfo.h"
#ifdef with_WrapCall
using WrapCall::ModcmdCallInfo;
#endif

int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	switch ( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			g_hInstance = hInstance;
#if _DEBUG
			if ( GetKeyState(VK_SHIFT) & 0x8000 ) { MessageBox(nullptr, "Attach Me!", "knowbug", MB_OK); }
#endif
			break;
		}
		case DLL_PROCESS_DETACH: debugbye(); break;
	}
	return TRUE;
}

EXPORT BOOL WINAPI debugini( HSP3DEBUG* p1, int p2, int p3, int p4 )
{
	ctx    = p1->hspctx;
	exinfo = ctx->exinfo2;

	g_dbginfo.reset(new DebugInfo(p1));
	g_config.initialize();

	Dialog::createMain();
	return 0;
}

EXPORT BOOL WINAPI debug_notice( HSP3DEBUG* p1, int p2, int p3, int p4 )
{
	switch ( p2 ) {
		// 実行が停止した (assert、ステップ実行の完了時など)
		case hpiutil::DebugNotice_Stop: {
			if (Knowbug::continueConditionalRun()) break;

			g_dbginfo->updateCurInf();
			Dialog::update();
			break;
		}
		case hpiutil::DebugNotice_Logmes:
			strcat_s(ctx->stmp, HSPCTX_REFSTR_MAX, "\r\n");
			Knowbug::logmes(ctx->stmp);
			break;
	}
	return 0;
}

void debugbye()
{
	Dialog::destroyMain();
}

namespace Knowbug
{

// ステップ実行中かどうかのフラグ
// 「脱出」等の条件付き実行は除く。
static bool bStepRunning = false;
bool isStepRunning() { return bStepRunning; }

// 条件付き実行の終了条件となる sublev
static int sublevOfGoal = -1;

HINSTANCE getInstance()
{
	return g_hInstance;
}

void runStop()
{
	g_dbginfo->setStepMode( HSPDEBUG_STOP );
}

void run()
{
	g_dbginfo->setStepMode(HSPDEBUG_RUN);
	bStepRunning = false;
}

void runStepIn() {
	// 本当のステップ実行でのみフラグが立つ
	bStepRunning = true;

	g_dbginfo->setStepMode(HSPDEBUG_STEPIN);
}

void runStepOver() { return runStepReturn( ctx->sublev ); }
void runStepOut()  { return runStepReturn( ctx->sublev - 1 ); }

// ctx->sublev == sublev になるまで step を繰り返す
void runStepReturn(int sublev)
{
	if ( sublev < 0 ) return run();	

	sublevOfGoal = sublev;
	bStepRunning = false;
	g_dbginfo->setStepMode(HSPDEBUG_STEPIN);
}

// 条件付き実行が継続されるか？
bool continueConditionalRun()
{
	if (sublevOfGoal >= 0) {
		if (ctx->sublev > sublevOfGoal) {
			g_dbginfo->setStepMode(HSPDEBUG_STEPIN); // stepin を繰り返す
			return true;
		} else {
			sublevOfGoal = -1;
		}
	}
	return false;
}

void logmes( char const* msg )
{
	VTRoot::log().append(msg);
}

void logmesWarning(char const* msg)
{
	g_dbginfo->updateCurInf();
	logmes(strf("warning: %s\r\nCurInf:%s\r\n",
		msg, g_dbginfo->getCurInfString()).c_str());
}

#ifdef with_WrapCall
void onBgnCalling(ModcmdCallInfo::shared_ptr_type const& callinfo)
{
	VTRoot::dynamic().onBgnCalling(callinfo);

	if ( Dialog::logsCalling() ) {
		string const logText = strf(
			"[CallBgn] %s\t%s]\r\n",
			callinfo->name(),
			DebugInfo::formatCurInfString(callinfo->fname, callinfo->line)
		);
		Knowbug::logmes(logText.c_str());
	}
}

void onEndCalling(ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT* ptr, vartype_t vtype)
{
	auto pResult = VTRoot::dynamic().onEndCalling(callinfo, ptr, vtype);

	if ( Dialog::logsCalling() ) {
		string const logText = strf(
			"[CallEnd] %s%s\r\n",
			callinfo->name(),
			(pResult ? ("-> " + pResult->lineformedString) : "")
		);
		Knowbug::logmes(logText.c_str());
	}
}
#endif

} //namespace Knowbug

// 公開API

EXPORT void WINAPI knowbug_writeVarinfoString(char const* name, PVal* pvalSrc, PVal* pvalDest)
{
	auto varinf = std::make_unique<CVarinfoText>();
	varinf->addVar(pvalSrc, name);

	if ( pvalDest->flag != HSPVAR_FLAG_STR ) {
		if ( pvalDest->offset != 0 ) puterror(HSPERR_TYPE_MISMATCH);
		exinfo->HspFunc_dim(pvalDest, HSPVAR_FLAG_STR, 0, 1, 0, 0, 0);
	}
	code_setva(pvalDest, pvalDest->offset, HSPVAR_FLAG_STR, varinf->getString().c_str());
}

/**
呼び出された関数の名前を refstr に出力する

@param n: n 個前の呼び出しの名前を得る
//*/
EXPORT void WINAPI knowbug_getCurrentModcmdName(char const* strNone, int n, char* prefstr)
{
#ifdef with_WrapCall
	auto range = WrapCall::getCallInfoRange();
	if ( std::distance(range.first, range.second) > n ) {
		auto const& callinfo = *(range.second - (n + 1));
		strcpy_s(prefstr, HSPCTX_REFSTR_MAX, callinfo->name().c_str());
	} else {
		strcpy_s(prefstr, HSPCTX_REFSTR_MAX, strNone);
	}
#else
	strcpy_s(prefstr, HSPCTX_REFSTR_MAX, strNone);
#endif
}
