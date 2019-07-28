
#include "pch.h"
#include <fstream>
#include "../hpiutil/dinfo.hpp"
#include "../knowbug_core/module/CStrWriter.h"
#include "../knowbug_core/module/strf.h"
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/DebugInfo.h"
#include "../knowbug_core/HspDebugApi.h"
#include "../knowbug_core/HspObjectWriter.h"
#include "../knowbug_core/HspRuntime.h"
#include "../knowbug_core/hsp_wrap_call.h"
#include "../knowbug_core/platform.h"
#include "../knowbug_core/source_files.h"
#include "../knowbug_core/StepController.h"
#include "knowbug_app.h"
#include "knowbug_config.h"
#include "knowbug_view.h"

static auto g_dll_instance = HINSTANCE{};

// FIXME: KnowbugApp に置く
std::unique_ptr<DebugInfo> g_dbginfo {};

// ランタイムとの通信
EXPORT BOOL WINAPI debugini(HSP3DEBUG* p1, int p2, int p3, int p4);
EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4);
static void debugbye();

auto create_source_file_repository(OsString&& common_path, hpiutil::DInfo const& debug_segment)
->std::unique_ptr<SourceFileRepository> {
	auto resolver = SourceFileResolver{};

	resolver.add_known_dir(std::move(common_path));

	for (auto file_ref_name : debug_segment.fileRefNames()) {
		resolver.add_file_ref_name(std::move(file_ref_name));
	}

	return std::make_unique<SourceFileRepository>(resolver.resolve());
}

class KnowbugAppImpl
	: public KnowbugApp
{
	std::unique_ptr<KnowbugConfig> config_;
	std::unique_ptr<KnowbugStepController> step_controller_;
	std::unique_ptr<SourceFileRepository> source_file_repository_;
	std::unique_ptr<HspRuntime> hsp_runtime_;
	std::unique_ptr<KnowbugView> view_;

public:
	KnowbugAppImpl(
		std::unique_ptr<KnowbugConfig> config,
		std::unique_ptr<KnowbugStepController> step_controller,
		std::unique_ptr<SourceFileRepository> source_file_repository,
		std::unique_ptr<HspRuntime> hsp_runtime,
		std::unique_ptr<KnowbugView> view
	)
		: config_(std::move(config))
		, step_controller_(std::move(step_controller))
		, source_file_repository_(std::move(source_file_repository))
		, hsp_runtime_(std::move(hsp_runtime))
		, view_(std::move(view))
	{
	}

	auto view() -> KnowbugView& override {
		return *view_;
	}

	void initialize() {
		wc_initialize(hsp_runtime_->wc_debugger());

		view().initialize();
	}

	void did_hsp_pause() {
		if (step_controller_->continue_step_running()) return;

		hsp_runtime_->update_location();
		view().update_source_edit(to_os(hsp_runtime_->objects().script_to_current_location_summary()));
		view().update();
	}

	void did_hsp_logmes(HspStringView const& text) {
		hsp_runtime_->logger().append(to_utf8(text));
		hsp_runtime_->logger().append(as_utf8(u8"\r\n"));

		view().did_log_change();
	}

	void step_run(StepControl const& step_control) override {
		step_controller_->update(step_control);
	}

	void add_object_text_to_log(HspObjectPath const& path) override {
		auto&& objects = hsp_runtime_->objects();

		// FIXME: 共通化
		auto writer = CStrWriter{};
		HspObjectWriter{ objects, writer }.write_table_form(path);
		auto text = as_utf8(writer.finish());

		hsp_runtime_->logger().append(text);
	}

	void clear_log() override {
		hsp_runtime_->logger().clear();
	}

	auto do_save_log(OsStringView const& file_path) -> bool {
		auto&& content = hsp_runtime_->logger().content();

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
		auto file_opt = hsp_runtime_->objects().script_to_file_id();
		if (!file_opt) {
			// FIXME: 何らかの警告を出す
			return;
		}

		auto&& full_path_opt = source_file_repository_->file_to_full_path(*file_opt);
		if (!full_path_opt) {
			// FIXME: 何らかの警告を出す
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
	auto api = HspDebugApi{ p1 };

	// グローバル変数の初期化:

	ctx    = api.context();
	exinfo = api.exinfo();

	auto debug_info = std::make_unique<DebugInfo>(p1);

	auto config = KnowbugConfig::create();

	auto step_controller = std::make_unique<KnowbugStepController>(api.context(), *debug_info);

	auto const& debug_segment = hpiutil::DInfo::instance();

	auto source_file_repository = create_source_file_repository(config->commonPath(), debug_segment);

	auto hsp_runtime = std::make_unique<HspRuntime>(std::move(api), *debug_info, *source_file_repository);

	auto view = KnowbugView::create(*config, g_dll_instance, hsp_runtime->objects(), hsp_runtime->object_tree());

	g_dbginfo = std::move(debug_info);
	g_app = std::make_shared<KnowbugAppImpl>(
		std::move(config),
		std::move(step_controller),
		std::move(source_file_repository),
		std::move(hsp_runtime),
		std::move(view)
	);

	// 起動処理:

	if (auto&& app = g_app) {
		app->initialize();
	}
	return 0;
}

EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4) {
	if (auto&& app_opt = g_app) {
		switch (p2) {
		// 実行が停止した (assert、ステップ実行の完了時など)
		case hpiutil::DebugNotice_Stop: {
			app_opt->did_hsp_pause();
			break;
		}
		case hpiutil::DebugNotice_Logmes:
			app_opt->did_hsp_logmes(as_hsp(ctx->stmp));
			break;
		}
	}
	return 0;
}

void debugbye() {
	if (auto&& app_opt = g_app) {
		app_opt->auto_save_log();
	}

	g_app.reset();
}
