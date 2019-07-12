
#include <fstream>
#include <winapifamily.h>
#include "module/CStrBuf.h"
#include "module/CStrWriter.h"
#include "encoding.h"
#include "main.h"
#include "module/strf.h"
#include "DebugInfo.h"
#include "config_mng.h"
#include "dialog.h"
#include "StepController.h"
#include "SourceFileResolver.h"
#include "HspRuntime.h"
#include "HspDebugApi.h"
#include "hpiutil/dinfo.hpp"
#include "HspObjectWriter.h"

static auto g_hInstance = HINSTANCE{};

// FIXME: KnowbugApp に置く
std::unique_ptr<DebugInfo> g_dbginfo {};

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

class KnowbugAppImpl
	: public KnowbugApp
{
	std::unique_ptr<KnowbugConfig> g_config;
	std::unique_ptr<KnowbugStepController> g_step_controller_;
	std::unique_ptr<SourceFileResolver> g_source_file_resolver;
	std::unique_ptr<HspRuntime> g_hsp_runtime;
	std::unique_ptr<KnowbugView> view_;

public:
	KnowbugAppImpl(
		std::unique_ptr<KnowbugConfig> config,
		std::unique_ptr<KnowbugStepController> step_controller,
		std::unique_ptr<SourceFileResolver> source_file_resolver,
		std::unique_ptr<HspRuntime> hsp_runtime,
		std::unique_ptr<KnowbugView> view
	)
		: g_config(std::move(config))
		, g_step_controller_(std::move(step_controller))
		, g_source_file_resolver(std::move(source_file_resolver))
		, g_hsp_runtime(std::move(hsp_runtime))
		, view_(std::move(view))
	{
	}

	auto view() -> KnowbugView& override {
		return *view_;
	}

	void did_hsp_pause() {
		if (g_step_controller_->continue_step_running()) return;

		g_dbginfo->updateCurInf();
		view().update_source_edit(to_os(g_hsp_runtime->objects().script_to_current_location_summary()));
		view().update();
	}

	void did_hsp_logmes(HspStringView const& text) {
		g_hsp_runtime->logger().append(to_utf8(text));
		g_hsp_runtime->logger().append(as_utf8(u8"\r\n"));

		view().did_log_change();
	}

	void step_run(StepControl step_control) override {
		g_step_controller_->update(step_control);
	}

	void add_object_text_to_log(HspObjectPath const& path) override {
		auto&& objects = g_hsp_runtime->objects();

		// FIXME: 共通化
		auto buffer = std::make_shared<CStrBuf>();
		buffer->limit(8000); // FIXME: 定数を共通化
		auto writer = CStrWriter{ buffer };
		HspObjectWriter{ objects, writer }.write_table_form(path);
		auto text = as_utf8(buffer->getMove());

		g_hsp_runtime->logger().append(text);
	}

	void clear_log() override {
		g_hsp_runtime->logger().clear();
	}

	auto do_save_log(OsStringView const& file_path) -> bool {
		auto&& content = g_hsp_runtime->logger().content();

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
		auto&& file_path = g_config->logPath;
		if (file_path.empty()) {
			return;
		}

		// NOTE: アプリの終了中なのでエラーを報告しない。
		do_save_log(file_path);
	}

	void open_current_script_file() override {
		auto file_ref_name = to_os(as_hsp(g_dbginfo->curPos().fileRefName()));
		auto&& full_path_opt = g_source_file_resolver->find_full_path(as_view(file_ref_name));
		if (full_path_opt) {
			ShellExecute(nullptr, TEXT("open"), full_path_opt->data(), nullptr, TEXT(""), SW_SHOWDEFAULT);
		}
	}

	void open_config_file() override {
		// ファイルが存在しなければ作成される。
		auto of = std::ofstream{ g_config->selfPath(), std::ios::app };

		ShellExecute(nullptr, TEXT("open"), g_config->selfPath().data(), nullptr, TEXT(""), SW_SHOWDEFAULT);
	}

	void open_knowbug_repository() override {
		auto url = TEXT("https://github.com/vain0x/knowbug");
		auto no_args = LPCTSTR{ nullptr };
		auto current_directory = LPCTSTR{ nullptr };
		ShellExecute(nullptr, TEXT("open"), url, no_args, current_directory, SW_SHOWDEFAULT);
	}
};

static auto g_app = std::shared_ptr<KnowbugAppImpl>{};

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
	auto api = HspDebugApi{ p1 };

	// グローバル変数の初期化:

	ctx    = api.context();
	exinfo = api.exinfo();

	auto debug_info = std::make_unique<DebugInfo>(p1);

	auto step_controller = std::make_unique<KnowbugStepController>(api.context(), *debug_info);

	auto config = KnowbugConfig::create();

	auto const& debug_segment = hpiutil::DInfo::instance();

	auto source_file_resolver = std::make_unique<SourceFileResolver>(config->commonPath(), debug_segment);

	auto hsp_runtime = std::make_unique<HspRuntime>(std::move(api), *debug_info, *source_file_resolver);

	auto view = KnowbugView::create(*config, g_hInstance, hsp_runtime->objects(), hsp_runtime->object_tree());

	g_dbginfo = std::move(debug_info);
	g_app = std::make_shared<KnowbugAppImpl>(
		std::move(config),
		std::move(step_controller),
		std::move(source_file_resolver),
		std::move(hsp_runtime),
		std::move(view)
	);

	// 起動処理:

	if (auto&& app = g_app) {
		app->view().initialize();
	}
	return 0;
}

EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4)
{
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

void debugbye()
{
	if (auto&& app_opt = g_app) {
		app_opt->auto_save_log();
	}

	g_app.reset();
}

namespace Knowbug
{
	auto get_app() -> std::shared_ptr<KnowbugApp> {
		return g_app;
	}

#ifdef with_WrapCall
void onBgnCalling(ModcmdCallInfo::shared_ptr_type const& callinfo) {
}

void onEndCalling(ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT* ptr, vartype_t vtype) {
}
#endif

} //namespace Knowbug
