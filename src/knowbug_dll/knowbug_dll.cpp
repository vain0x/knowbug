
#include "pch.h"
#include <fstream>
#include "../hspsdk/hsp3plugin.h"
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/hsp_object_list.h"
#include "../knowbug_core/hsp_object_path.h"
#include "../knowbug_core/hsp_object_writer.h"
#include "../knowbug_core/hsp_objects.h"
#include "../knowbug_core/hsp_wrap_call.h"
#include "../knowbug_core/platform.h"
#include "../knowbug_core/source_files.h"
#include "../knowbug_core/string_writer.h"
#include "knowbug_dll.h"
#include "knowbug_server.h"

class KnowbugAppImpl;

static auto s_fs = WindowsFileSystemApi{};
static auto s_dll_instance = HINSTANCE{};
static auto s_debug_opt = std::optional<HSP3DEBUG*>{};

// HSPCTX::msgfunc の型
using HspMsgFunc = void(*)(HSPCTX*);
static auto s_msgfunc_orig = (HspMsgFunc)nullptr;

// HSPCTX::msgfunc を差し替えるもの
static void knowbug_msgfunc(HSPCTX* ctx);

// 条件付きステップ実行の状態
// (値が0以上なら条件付きステップ実行の処理中。ゴールの sublev に戻るまで続ける)
static auto s_sublev_goal = -1;

// ランタイムとの通信
EXPORT BOOL WINAPI debugini(HSP3DEBUG* p1, int p2, int p3, int p4);
EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4);
static void debugbye();

// -----------------------------------------------
// バージョン
// -----------------------------------------------

static constexpr auto KNOWBUG_VERSION = u8"v2.2.2";

#ifdef _M_X64
static constexpr auto KNOWBUG_PLATFORM_SUFFIX = u8" (x64)";
#else //defined(_M_X64)
static constexpr auto KNOWBUG_PLATFORM_SUFFIX = u8"";
#endif

#ifdef HSP3_UTF8
static constexpr auto KNOWBUG_ENCODING_SUFFIX = u8" (UTF-8)";
#else
static constexpr auto KNOWBUG_ENCODING_SUFFIX = u8"";
#endif

auto knowbug_version() -> std::u8string {
	auto suffix = std::u8string{ KNOWBUG_VERSION };
	suffix += KNOWBUG_PLATFORM_SUFFIX;
	suffix += KNOWBUG_ENCODING_SUFFIX;
	return suffix;
}

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

static void post_null() {
	PostMessage(NULL, WM_NULL, 0, 0); // post_null
}

class KnowbugAppImpl
	: public KnowbugApp
	, public KnowbugReceiver
{
	HSP3DEBUG* debug_;
	std::unique_ptr<HspObjects> objects_;
	std::shared_ptr<KnowbugServer> server_;

	// from server
	std::u8string pending_logmes_;
	int pending_runmode_;

	// クライアントからの停止・ステップ要求で未解決のもの
	//
	// HACK: 停止が要求されてから実際にHSPランタイムが停止状態になるまでの間に
	//       logmes 命令が実行された場合、runmode が上書きされることがある。
	//       その対処として、logmes の実行後にこのサーバー自身にメッセージをポストし、
	//       そのメッセージの解決時に要求されている runmode を再設定する)
	std::optional<int> requested_mode_;

	std::unique_ptr<HspObjectListEntity> object_list_entity_;

public:
	KnowbugAppImpl(
		std::unique_ptr<HspObjects> objects
	)
		: debug_{ *s_debug_opt }
		, objects_(std::move(objects))
		, server_(KnowbugServer::create(*s_debug_opt, this->objects(), s_dll_instance, *this))
		, pending_logmes_{}
		, pending_runmode_{ HSPDEBUG_RUN }
		, object_list_entity_{ HspObjectListEntity::create() }
	{
	}

	auto objects() -> HspObjects& {
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
		if (s_sublev_goal >= 0) {
			s_sublev_goal = -1;
		}

		//server().debuggee_did_stop();
		if (!client_ready_) {
			pending_runmode_ = HSPDEBUG_STOP;
			return;
		}

		requested_mode_ = std::nullopt;

		server_->get_sender().sender(send_stopped_event();
	}

	void did_hsp_logmes(HspStringView const& text) {
		//server().logmes(text);
		auto utf8_text = to_utf8(text);

		if (!server_->client_ready_) {
			if (!pending_logmes_.empty()) {
				pending_logmes_ += u8"\r\n";
			}
			pending_logmes_ += utf8_text;
			return;
		}

		server_->get_sender().send_output_event(utf8_text);

		// (requested_mode_ の説明を参照)
		if (requested_mode_.has_value() && hidden_window_opt_) {
			PostMessage(hidden_window_opt_->get(), WM_APP, 0, 0);
		}

		objects().log_do_append(to_utf8(text));
		objects().log_do_append(u8"\r\n");
	}

	// クライアントからのメッセージを処理する

	void client_did_initialize(IncomingCtx incoming) override {
		incoming.sender.send_initialized_event();

		// クライアントとの接続が確立する前にランタイムから受け取っていたイベントを伝える
		if (pending_runmode_ != HSPDEBUG_RUN) {
			incoming.sender.send_stopped_event();
		}
		if (!pending_logmes_.empty()) {
			incoming.sender.send_output_event(std::exchange(pending_logmes_, u8""));
		}
	}

	void client_did_terminate(IncomingCtx incoming) override {
		PostQuitMessage(EXIT_SUCCESS);
	}

	void client_did_step_continue(IncomingCtx incoming) override {
		hsx::debug_do_set_mode(HSPDEBUG_RUN, debug_);
		post_null();

		incoming.sender.send_continued_event();
	}

	void client_did_step_pause(IncomingCtx incoming) override {
		requested_mode_ = (int)HSPDEBUG_STOP;
		hsx::debug_do_set_mode(HSPDEBUG_STOP, debug_);
		post_null();
	}

	void client_did_step_in(IncomingCtx incoming) override {
		hsx::debug_do_set_mode(HSPDEBUG_STEPIN, debug_);
		post_null();

		incoming.sender.send_continued_event();
	}

	void client_did_step_over(IncomingCtx incoming) override {
		knowbug_step_over(debug_);
		incoming.sender.send_continued_event();
	}

	void client_did_step_out(IncomingCtx incoming) override {
		knowbug_step_out(debug_);
		incoming.sender.send_continued_event();
	}

	void client_did_location_update(IncomingCtx incoming) override {
		objects().script_do_update_location();

		auto source_file_id = objects().script_to_current_file().value_or(0);
		auto line_index = objects().script_to_current_line();
		incoming.sender.send_location_event(source_file_id, line_index);
	}

	void client_did_source(IncomingCtx incoming, int source_file_id) {
		if (source_file_id < 0) {
			assert(false && u8"bad source_file_id");
			return;
		}

		auto full_path_opt = objects().source_file_to_full_path(source_file_id);
		auto content_opt = objects().source_file_to_content(source_file_id);

		incoming.sender.send_source_event((std::size_t)source_file_id, full_path_opt, content_opt);
	}

	void client_did_list_update(IncomingCtx incoming) override {
		auto diff = object_list_entity_->update(objects());
		incoming.sender.send_list_updated_events(std::move(diff));
	}

	void client_did_list_toggle_expand(IncomingCtx incoming, int object_id) override {
		if (object_id < 0) {
			assert(false && u8"bad object_id");
			return;
		}

		object_list_entity_->toggle_expand((std::size_t)object_id);
	}

	void client_did_list_details(IncomingCtx incoming, int object_id) override {
		if (object_id < 0) {
			assert(false && u8"bad object_id");
			return;
		}

		auto text_opt = std::optional<std::u8string>{};

		auto path_opt = object_list_entity_->object_id_to_path(object_id);
		if (path_opt) {
			auto string_writer = StringWriter{};
			HspObjectWriter{ objects(), string_writer }.write_table_form(**path_opt);
			text_opt = string_writer.finish();
		}

		incoming.sender.send_list_details_event((std::size_t)object_id, text_opt);
	}
};

// -----------------------------------------------

void knowbug_step_over(HSP3DEBUG* debug) {
	s_sublev_goal = ctx->sublev;
	debug->dbg_set(HSPDEBUG_STEPIN);
	PostMessage(NULL, WM_NULL, 0, 0); // post_null
}

void knowbug_step_out(HSP3DEBUG* debug) {
	s_sublev_goal = ctx->sublev - 1;
	debug->dbg_set(HSPDEBUG_STEPIN);
	PostMessage(NULL, WM_NULL, 0, 0); // post_null
}

// -----------------------------------------------

static auto s_app = std::shared_ptr<KnowbugAppImpl>{};

auto KnowbugApp::instance() -> std::shared_ptr<KnowbugApp> {
	return s_app;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH: {
		s_dll_instance = hInstance;
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

	s_debug_opt = debug;

	auto common_dir = get_hsp_dir();
	common_dir += TEXT("/common/");

	// :thinking_face:
	auto resolver = SourceFileResolver{ s_fs };
	auto objects_builder = HspObjectsBuilder{};
	resolver.add_known_dir(std::move(common_dir));
	objects_builder.read_debug_segment(resolver, ctx);
	auto source_file_repository = std::make_unique<SourceFileRepository>(resolver.resolve());
	auto objects = std::make_unique<HspObjects>(objects_builder.finish(debug, std::move(source_file_repository)));

	s_app = std::make_shared<KnowbugAppImpl>(
		std::move(objects)
	);

	// hspctx->msgfunc を差し替える
	s_msgfunc_orig = ctx->msgfunc;
	ctx->msgfunc = knowbug_msgfunc;

	// 起動処理:

	if (auto app = std::shared_ptr{ s_app }) {
		app->initialize();
	}

	return 0;
}

EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4) {
	if (auto app = std::shared_ptr{ s_app }) {
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
	if (auto app = std::shared_ptr{ s_app }) {
		app->will_exit();
	}

	s_app.reset();
}

// HSPCTX::msgfunc を差し替えるもの
void knowbug_msgfunc(HSPCTX* ctx)
{
	// 条件付きステップ実行の継続処理
	if (s_debug_opt.has_value() && s_sublev_goal >= 0) {
		if (ctx->sublev > s_sublev_goal) {
			s_debug_opt.value()->dbg_set(HSPDEBUG_STEPIN);
		} else {
			s_sublev_goal = -1;
		}
	}

	s_msgfunc_orig(ctx);
}
