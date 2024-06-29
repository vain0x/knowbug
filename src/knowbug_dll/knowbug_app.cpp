
#include "pch.h"
#include <fstream>
#include "../hspsdk/hsp3plugin.h"
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/hsp_object_path.h"
#include "../knowbug_core/hsp_object_writer.h"
#include "../knowbug_core/hsp_objects.h"
#include "../knowbug_core/hsp_wrap_call.h"
#include "../knowbug_core/platform.h"
#include "../knowbug_core/source_files.h"
#include "../knowbug_core/step_controller.h"
#include "../knowbug_core/string_writer.h"
#include "knowbug_app.h"
#include "knowbug_server.h"

class KnowbugAppImpl;

static auto g_fs = WindowsFileSystemApi{};
static auto g_dll_instance = HINSTANCE{};
static auto g_debug_opt = std::optional<HSP3DEBUG*>{};

// ランタイムとの通信
EXPORT BOOL WINAPI debugini(HSP3DEBUG* p1, int p2, int p3, int p4);
EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4);
static void debugbye();

// -----------------------------------------------
// KnowbugApp
// -----------------------------------------------

static auto get_hsp_dir() -> OsString {
	// DLL の絶対パスを取得する。
	auto buffer = std::array<TCHAR, MAX_PATH>{};
	GetModuleFileName(GetModuleHandle(nullptr), buffer.data(), (DWORD)buffer.size());
	auto full_path = OsString{ buffer.data() };

	// ファイル名の部分を削除
	while (!full_path.empty()) {
		auto last = full_path[full_path.length() - 1];
		if (last == TEXT('/') || last == TEXT('\\')) {
			break;
		}

		full_path.pop_back();
	}

	return full_path;
}

class KnowbugAppImpl
	: public KnowbugApp
{
	friend class HspObjectTreeObserverImpl;

	std::unique_ptr<KnowbugStepController> step_controller_;
	std::unique_ptr<HspObjects> objects_;
	std::shared_ptr<KnowbugServer> server_;

public:
	KnowbugAppImpl(
		std::unique_ptr<KnowbugStepController> step_controller,
		std::unique_ptr<HspObjects> objects
	)
		: step_controller_(std::move(step_controller))
		, objects_(std::move(objects))
		, server_(KnowbugServer::create(*g_debug_opt, this->objects(), g_dll_instance, *step_controller_))
	{
	}

	auto objects() ->HspObjects& {
		return *objects_;
	}

	auto server() -> KnowbugServer& {
		return *server_;
	}

	void initialize() {
		objects().initialize();

		server().start();
	}

	void will_exit() {
		server().will_exit();
	}

	void did_hsp_pause() {
		if (step_controller_->continue_step_running()) {
			// HACK: すべてのウィンドウに無意味なメッセージを送信する。
			//       HSP のウィンドウがこれを受信したとき、デバッグモードの変化が再検査されて、
			//       ステップ実行モードが変化したことに気づいてくれる (実装依存)。
			PostMessage(HWND_BROADCAST, WM_NULL, 0, 0);
			return;
		}

		server().debuggee_did_stop();
	}

	void did_hsp_logmes(HspStringView const& text) {
		server().logmes(text);

		objects().log_do_append(to_utf8(text));
		objects().log_do_append(u8"\r\n");
	}

	void step_run(StepControl const& step_control) override {
		step_controller_->update(step_control);
	}
};

// -----------------------------------------------

static auto g_app = std::shared_ptr<KnowbugAppImpl>{};

auto KnowbugApp::instance() -> std::shared_ptr<KnowbugApp> {
	return g_app;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved) {
	switch ( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			g_dll_instance = hInstance;
#if _DEBUG
			if (GetKeyState(VK_SHIFT) & 0x8000) {
				MessageBox(nullptr, TEXT("Ctrl+Alt+P でプロセス hsp3.exe にアタッチし、デバッグを開始してください。"), TEXT("knowbug"), MB_OK);
			}
#endif
			break;
		}
		case DLL_PROCESS_DETACH: debugbye(); break;
	}
	return TRUE;
}

EXPORT BOOL WINAPI debugini(HSP3DEBUG* p1, int p2, int p3, int p4) {
	auto debug = p1;

	// グローバル変数の初期化:

	ctx = p1->hspctx;
	exinfo = ctx->exinfo2;

	g_debug_opt = debug;

	auto step_controller = std::make_unique<KnowbugStepController>(debug);

	auto common_dir = get_hsp_dir();
	common_dir += TEXT("/common/");

	// :thinking_face:
	auto resolver = SourceFileResolver{ g_fs };
	auto objects_builder = HspObjectsBuilder{};
	resolver.add_known_dir(std::move(common_dir));
	objects_builder.read_debug_segment(resolver, ctx);
	auto source_file_repository = std::make_unique<SourceFileRepository>(resolver.resolve());
	auto objects = std::make_unique<HspObjects>(objects_builder.finish(debug, std::move(source_file_repository)));

	g_app = std::make_shared<KnowbugAppImpl>(
		std::move(step_controller),
		std::move(objects)
	);

	// 起動処理:

	if (auto app = std::shared_ptr{ g_app }) {
		app->initialize();
	}

	return 0;
}

EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4) {
	if (auto app = std::shared_ptr{ g_app }) {
		switch (p2) {
		case HSX_DEBUG_NOTICE_STOP:
			app->did_hsp_pause();
			break;

		case HSX_DEBUG_NOTICE_LOGMES:
			app->did_hsp_logmes(as_hsp(ctx->stmp));
			break;
		}
	}
	return 0;
}

void debugbye() {
	if (auto app = std::shared_ptr{ g_app }) {
		app->will_exit();
	}

	g_app.reset();
}
