
#include <winapifamily.h>
#include "main.h"
#include "module/strf.h"
#include "DebugInfo.h"
#include "VarTreeNodeData.h"
#include "CVarinfoText.h"
#include "config_mng.h"
#include "dialog.h"
#include "StepController.h"

static auto g_hInstance = HINSTANCE {};
std::unique_ptr<DebugInfo> g_dbginfo {};
static std::unique_ptr<KnowbugStepController> g_step_controller_;

// ランタイムとの通信
EXPORT BOOL WINAPI debugini(HSP3DEBUG* p1, int p2, int p3, int p4);
EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4);
static void debugbye();

// WrapCall 関連
#include "WrapCall/WrapCall.h"
#include "WrapCall/ModcmdCallInfo.h"
#ifdef with_WrapCall
using WrapCall::ModcmdCallInfo;
#endif

auto WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved) -> int
{
	switch ( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			g_hInstance = hInstance;
#if _DEBUG
			if (GetKeyState(VK_SHIFT) & 0x8000) { MessageBox(nullptr, TEXT("Attach Me!"), TEXT("knowbug"), MB_OK); }
#endif
			break;
		}
		case DLL_PROCESS_DETACH: debugbye(); break;
	}
	return TRUE;
}

EXPORT BOOL WINAPI debugini(HSP3DEBUG* p1, int p2, int p3, int p4)
{
	ctx    = p1->hspctx;
	exinfo = ctx->exinfo2;

	KnowbugConfig::initialize();
	g_dbginfo.reset(new DebugInfo(p1));

	g_step_controller_ = std::make_unique<KnowbugStepController>(ctx, *g_dbginfo);

	Dialog::createMain();
	return 0;
}

EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4)
{
	switch ( p2 ) {
		// 実行が停止した (assert、ステップ実行の完了時など)
		case hpiutil::DebugNotice_Stop: {
			if ( Knowbug::continueConditionalRun() ) break;

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
	auto getInstance() -> HINSTANCE {
		return g_hInstance;
	}

	void step_run(StepControl step_control) {
		g_step_controller_->update(step_control);
	}

	bool continueConditionalRun() {
		return g_step_controller_->continue_step_running();
	}

void logmes( char const* msg )
{
	VTRoot::log().append(msg);
}

void logmesWarning(char const* msg)
{
	g_dbginfo->updateCurInf();
	logmes(strf("warning: %s\r\nCurInf:%s\r\n"
			, msg, g_dbginfo->getCurInfString()).c_str());
}

#ifdef with_WrapCall
void onBgnCalling(ModcmdCallInfo::shared_ptr_type const& callinfo)
{
	VTRoot::dynamic().onBgnCalling(callinfo);

	if ( Dialog::logsCalling() ) {
		auto logText =
			strf
			( "[CallBgn] %s\t%s]\r\n"
			, callinfo->name()
			, callinfo->callerPos.toString()
			);
		Knowbug::logmes(logText.c_str());
	}
}

void onEndCalling(ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT* ptr, vartype_t vtype)
{
	VTRoot::dynamic().onEndCalling(callinfo, ptr, vtype);

	if ( Dialog::logsCalling() ) {
		auto logText =
			strf
			( "[CallEnd] %s\r\n"
			, callinfo->name()
			);
		Knowbug::logmes(logText.c_str());
	}
}
#endif

} //namespace Knowbug
