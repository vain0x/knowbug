
#include "pch.h"
#include <fstream>
#include "../hspsdk/hsp3plugin.h"
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/hsp_object_tree.h"
#include "../knowbug_core/hsp_object_writer.h"
#include "../knowbug_core/hsp_objects.h"
#include "../knowbug_core/hsp_wrap_call.h"
#include "../knowbug_core/platform.h"
#include "../knowbug_core/source_files.h"
#include "../knowbug_core/step_controller.h"
#include "../knowbug_core/string_writer.h"
#include "knowbug_app.h"
#include "knowbug_config.h"
#include "knowbug_view.h"

static auto g_fs = WindowsFileSystemApi{};
static auto g_dll_instance = HINSTANCE{};

// ランタイムとの通信
EXPORT BOOL WINAPI debugini(HSP3DEBUG* p1, int p2, int p3, int p4);
EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4);
static void debugbye();

class KnowbugAppImpl
	: public KnowbugApp
{
	std::unique_ptr<KnowbugConfig> config_;
	std::unique_ptr<KnowbugStepController> step_controller_;
	std::unique_ptr<HspObjects> objects_;
	std::unique_ptr<HspObjectTree> object_tree_;
	std::unique_ptr<KnowbugView> view_;

public:
	KnowbugAppImpl(
		std::unique_ptr<KnowbugConfig> config,
		std::unique_ptr<KnowbugStepController> step_controller,
		std::unique_ptr<HspObjects> objects,
		std::unique_ptr<HspObjectTree> object_tree,
		std::unique_ptr<KnowbugView> view
	)
		: config_(std::move(config))
		, step_controller_(std::move(step_controller))
		, objects_(std::move(objects))
		, object_tree_(std::move(object_tree))
		, view_(std::move(view))
	{
	}

	auto view() -> KnowbugView& override {
		return *view_;
	}

	void initialize() {
		objects().initialize();

		view().initialize();
	}

	void will_exit() {
		auto_save_log();

		view().will_exit();
	}

	void did_hsp_pause() {
		if (step_controller_->continue_step_running()) {
			// HACK: すべてのウィンドウに無意味なメッセージを送信する。
			//       HSP のウィンドウがこれを受信したとき、デバッグモードの変化が再検査されて、
			//       ステップ実行モードが変化したことに気づいてくれる (実装依存)。
			PostMessage(HWND_BROADCAST, WM_NULL, 0, 0);
			return;
		}

		update_view();
	}

	void did_hsp_logmes(HspStringView const& text) {
		objects().log_do_append(to_utf8(text));
		objects().log_do_append(as_utf8(u8"\r\n"));

		view().did_log_change();
	}

	void update_view() override {
		objects().script_do_update_location();
		view().update_source_edit(to_os(objects().script_to_current_location_summary()));
		view().update();
	}

	void step_run(StepControl const& step_control) override {
		step_controller_->update(step_control);
	}

	void add_object_text_to_log(HspObjectPath const& path) override {
		// FIXME: 共通化
		auto writer = StringWriter{};
		HspObjectWriter{ objects(), writer }.write_table_form(path);
		auto text = writer.finish();

		objects().log_do_append(text);
	}

	void clear_log() override {
		objects().log_do_clear();
	}

	auto do_save_log(OsStringView const& file_path) -> bool {
		auto&& content = objects().log_to_content();

		auto file_stream = std::ofstream{ file_path.data() };
		file_stream.write(as_native(content).data(), content.size());
		auto success = file_stream.good();

		return success;
	}

	void save_log() override {
		auto&& file_path_opt = view().select_save_log_file();
		if (!file_path_opt) {
			return;
		}

		auto success = do_save_log(*file_path_opt);
		if (!success) {
			view().notify_save_failure();
		}
	}

	void auto_save_log() {
		auto&& file_path = config_->logPath;
		if (file_path.empty()) {
			return;
		}

		// NOTE: アプリの終了中なのでエラーを報告しない。
		do_save_log(file_path);
	}

	void open_current_script_file() override {
		auto full_path_opt = objects().script_to_full_path();
		if (!full_path_opt) {
			view().notify_open_file_failure();
			return;
		}

		ShellExecute(nullptr, TEXT("open"), full_path_opt->data(), nullptr, TEXT(""), SW_SHOWDEFAULT);
	}

	void open_config_file() override {
		// ファイルが存在しなければ作成される。
		auto of = std::ofstream{ config_->selfPath(), std::ios::app };

		ShellExecute(nullptr, TEXT("open"), config_->selfPath().data(), nullptr, TEXT(""), SW_SHOWDEFAULT);
	}

	void open_knowbug_repository() override {
		auto url = TEXT("https://github.com/vain0x/knowbug");
		auto no_args = LPCTSTR{ nullptr };
		auto current_directory = LPCTSTR{ nullptr };
		ShellExecute(nullptr, TEXT("open"), url, no_args, current_directory, SW_SHOWDEFAULT);
	}

private:
	auto objects() ->HspObjects& {
		return *objects_;
	}

	auto object_tree() ->HspObjectTree& {
		return *object_tree_;
	}
};

static auto g_app = std::shared_ptr<KnowbugAppImpl>{};

auto KnowbugApp::instance() -> std::shared_ptr<KnowbugApp> {
	return g_app;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved) {
	switch ( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			g_dll_instance = hInstance;
#if _DEBUG
			if (GetKeyState(VK_SHIFT) & 0x8000) { MessageBox(nullptr, TEXT("Attach Me!"), TEXT("knowbug"), MB_OK); }
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

	auto config = KnowbugConfig::create();

	auto step_controller = std::make_unique<KnowbugStepController>(debug);

	// :thinking_face:
	auto resolver = SourceFileResolver{ g_fs };
	auto objects_builder = HspObjectsBuilder{};
	resolver.add_known_dir(config->commonPath());
	objects_builder.read_debug_segment(resolver, ctx);
	auto source_file_repository = std::make_unique<SourceFileRepository>(resolver.resolve());
	auto objects = std::make_unique<HspObjects>(objects_builder.finish(debug, std::move(source_file_repository)));

	auto object_tree = HspObjectTree::create(*objects);

	auto view = KnowbugView::create(*config, g_dll_instance, *objects, *object_tree);

	g_app = std::make_shared<KnowbugAppImpl>(
		std::move(config),
		std::move(step_controller),
		std::move(objects),
		std::move(object_tree),
		std::move(view)
	);

	// 起動処理:

	if (auto&& app = g_app) {
		app->initialize();
	}
	return 0;
}

EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4) {
	auto kind = (hsx::DebugNoticeKind)p2;

	if (auto&& app_opt = g_app) {
		switch (kind) {
		case hsx::DebugNoticeKind::Stop:
			app_opt->did_hsp_pause();
			break;

		case hsx::DebugNoticeKind::Logmes:
			app_opt->did_hsp_logmes(as_hsp(ctx->stmp));
			break;
		}
	}
	return 0;
}

void debugbye() {
	if (auto&& app_opt = g_app) {
		app_opt->will_exit();
	}

	g_app.reset();
}
