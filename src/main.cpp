
#include <fstream>
#include <winapifamily.h>
#include "encoding.h"
#include "main.h"
#include "module/strf.h"
#include "DebugInfo.h"
#include "VarTreeNodeData.h"
#include "CVarinfoText.h"
#include "config_mng.h"
#include "dialog.h"
#include "StepController.h"
#include "Logger.h"
#include "SourceFileResolver.h"

static auto g_hInstance = HINSTANCE {};
std::unique_ptr<DebugInfo> g_dbginfo {};
static std::unique_ptr<KnowbugStepController> g_step_controller_;
static std::shared_ptr<Logger> g_logger;
static std::shared_ptr<SourceFileResolver> g_source_file_resolver;

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
	// グローバル変数の初期化:

	ctx    = p1->hspctx;
	exinfo = ctx->exinfo2;

	g_logger = std::make_shared<Logger>();

	g_dbginfo.reset(new DebugInfo(p1));

	g_step_controller_ = std::make_unique<KnowbugStepController>(ctx, *g_dbginfo);

	KnowbugConfig::initialize();

	auto const& debug_segment = hpiutil::DInfo::instance();

	g_source_file_resolver = std::make_shared<SourceFileResolver>(g_config->commonPath(), debug_segment);

	// 起動時の処理:

	g_logger->enable_auto_save(g_config->logPath.as_ref());

	Dialog::createMain(debug_segment);
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
			g_logger->append_line(HspStringView{ ctx->stmp }.to_os_string().as_ref());
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

	auto get_logger() -> std::shared_ptr<Logger> {
		return g_logger;
	}

	auto get_source_file_resolver() -> std::shared_ptr<SourceFileResolver> {
		return g_source_file_resolver;
	}

	void step_run(StepControl step_control) {
		g_step_controller_->update(step_control);
	}

	bool continueConditionalRun() {
		return g_step_controller_->continue_step_running();
	}

	void logmes(OsStringView const& msg) {
		g_logger->append(msg);
	}

	void logmes( char const* msg ) {
		logmes(HspStringView{ msg }.to_os_string().as_ref());
	}

	void logmesWarning(OsStringView const& msg) {
		g_dbginfo->updateCurInf();
		auto execution_location = HspStringView{ g_dbginfo->getCurInfString().data() }.to_os_string();

		g_logger->append_warning(msg, execution_location.as_ref());
	}

	void logmesWarning(char const* msg) {
		logmesWarning(HspStringView{ msg }.to_os_string().as_ref());
	}

	void open_current_script_file() {
		auto file_ref_name = HspStringView{ g_dbginfo->curPos().fileRefName() }.to_os_string();
		OsStringView full_path;
		auto ok = Knowbug::get_source_file_resolver()->find_full_path(file_ref_name.as_ref(), full_path);
		if (ok) {
			ShellExecute(nullptr, TEXT("open"), full_path.data(), nullptr, TEXT(""), SW_SHOWDEFAULT);
		}
	}

	void open_config_file() {
		// ファイルが存在しなければ作成される。
		auto of = std::ofstream{ g_config->selfPath(), std::ios::app };

		ShellExecute(nullptr, TEXT("open"), g_config->selfPath().data(), nullptr, TEXT(""), SW_SHOWDEFAULT);
	}

	void open_knowbug_repository() {
		auto url = TEXT("https://github.com/vain0x/knowbug");
		auto no_args = LPCTSTR{ nullptr };
		auto current_directory = LPCTSTR{ nullptr };
		ShellExecute(nullptr, TEXT("open"), url, no_args, current_directory, SW_SHOWDEFAULT);
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
