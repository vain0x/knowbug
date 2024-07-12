#include "pch.h"

#include <array>
#include <memory>
#include <deque>
#include <mutex>
#include <optional>
#include <unordered_set>
#include <vector>
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/hsp_object_path.h"
#include "../knowbug_core/hsp_object_writer.h"
#include "../knowbug_core/hsp_objects.h"
#include "../knowbug_core/hsx.h"
#include "../knowbug_core/knowbug_protocol.h"
#include "../knowbug_core/platform.h"
#include "../knowbug_core/step_controller.h"
#include "../knowbug_core/string_writer.h"
#include "hsp_object_list.h"
#include "knowbug_app.h"
#include "knowbug_server.h"
#include "logger.h"

class KnowbugServerImpl;

static constexpr auto MEMORY_BUFFER_SIZE = std::size_t{ 1024 * 1024 };

// -----------------------------------------------
// バージョン
// -----------------------------------------------

static constexpr auto KNOWBUG_VERSION = u8"v2.2.1";

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

static auto knowbug_version() -> std::u8string {
	auto suffix = std::u8string{ KNOWBUG_VERSION };
	suffix += KNOWBUG_PLATFORM_SUFFIX;
	suffix += KNOWBUG_ENCODING_SUFFIX;
	return suffix;
}

// -----------------------------------------------
// ヘルパー
// -----------------------------------------------

class Win32CloseHandleFn {
public:
	using pointer = HANDLE;

	void operator()(HANDLE p) {
		CloseHandle(p);
	}
};

class Win32DestroyWindowFn {
public:
	using pointer = HWND;

	void operator()(HWND p) {
		DestroyWindow(p);
	}
};

class Win32UnmapViewOfFileFn {
public:
	using pointer = LPVOID;

	void operator()(LPVOID p) {
		UnmapViewOfFile(p);
	}
};

using MemoryMappedFile = std::unique_ptr<HANDLE, Win32CloseHandleFn>;

using MemoryMappedFileView = std::unique_ptr<LPVOID, Win32UnmapViewOfFileFn>;

using PipeHandle = std::unique_ptr<HANDLE, Win32CloseHandleFn>;

using ProcessHandle = std::unique_ptr<HANDLE, Win32CloseHandleFn>;

using ThreadHandle = std::unique_ptr<HANDLE, Win32CloseHandleFn>;

using WindowHandle = std::unique_ptr<HWND, Win32DestroyWindowFn>;

class PipePair {
public:
	PipeHandle read_;
	PipeHandle write_;
};

static void fail_with(OsStringView reason) {
	MessageBox(HWND{}, to_owned(reason).data(), TEXT("knowbug"), MB_ICONWARNING);
	exit(EXIT_FAILURE);
}

// -----------------------------------------------
// 隠しウィンドウ
// -----------------------------------------------

static auto WINAPI process_hidden_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)->LRESULT;

static auto create_hidden_window(HINSTANCE instance) -> WindowHandle {
	static auto constexpr CLASS_NAME = TEXT("KnowbugHiddenWindowClass");
	static auto constexpr TITLE = TEXT("Knowbug Hidden Window");
	static auto constexpr STYLE = DWORD{};
	static auto constexpr POS_X = -1000;
	static auto constexpr POS_Y = -1000;
	static auto constexpr SIZE_X = 10;
	static auto constexpr SIZE_Y = 10;

	auto wndclass = WNDCLASS{};
	wndclass.lpfnWndProc = process_hidden_window;
	wndclass.hInstance = instance;
	wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndclass.lpszClassName = CLASS_NAME;
	RegisterClass(&wndclass);

	auto hwnd = CreateWindow(
		CLASS_NAME,
		TITLE,
		STYLE,
		POS_X,
		POS_Y,
		SIZE_X,
		SIZE_Y,
		HWND{},
		HMENU{},
		instance,
		LPVOID{}
	);
	if (!hwnd) {
		MessageBox(HWND{}, TEXT("デバッグウィンドウの初期化に失敗しました。(サーバーウィンドウを作成に失敗しました。)"), TEXT("knowbug"), MB_ICONERROR);
		exit(EXIT_FAILURE);
	}
	auto window_handle = WindowHandle{ hwnd };

	ShowWindow(window_handle.get(), SW_HIDE);
	return window_handle;
}

// -----------------------------------------------
// クライアントプロセス
// -----------------------------------------------

class KnowbugClientProcess {
public:
	ThreadHandle thread_handle_;
	ProcessHandle process_handle_;
};

// FIXME: knowbug_app と重複
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

static auto start_client_process(HWND server_hwnd) -> std::optional<KnowbugClientProcess> {
	// クライアントプロセスの起動コマンドラインを構成する。
	auto hsp_dir = get_hsp_dir();
	auto name = hsp_dir + TEXT("knowbug_client.exe");

	auto cmdline = OsString{ TEXT("\"") };
	cmdline += name;
	cmdline += TEXT("\"");

	{
		// OK: HWNDの値はintで表現できる
		// (https://learn.microsoft.com/ja-jp/windows/win32/winprog64/interprocess-communication)
		char buf[64] = "";
		sprintf_s(buf, "%d", (int)(std::uintptr_t)server_hwnd);
		cmdline += _T(" --server-hwnd=");
		cmdline += to_os(ascii_as_utf8(buf));
	}

	auto startup_info = STARTUPINFO{ sizeof(STARTUPINFO) };
	auto process_info = PROCESS_INFORMATION{};

	// クライアントプロセスを起動する。
	if (!CreateProcess(
		LPCTSTR{},
		cmdline.data(),
		LPSECURITY_ATTRIBUTES{},
		LPSECURITY_ATTRIBUTES{},
		TRUE,
		NORMAL_PRIORITY_CLASS,
		LPTSTR{},
		hsp_dir.data(),
		&startup_info,
		&process_info
	)) {
		debugf(u8"CreateProcess failed err=%d", GetLastError());
		return std::nullopt;
	}

	auto thread_handle = ThreadHandle{ process_info.hThread };
	auto process_handle = ProcessHandle{ process_info.hProcess };

	return KnowbugClientProcess{
		std::move(thread_handle),
		std::move(process_handle),
	};
}

// -----------------------------------------------
// メッセージ
// -----------------------------------------------

class Msg {
	int kind_;
	int wparam_;
	int lparam_;
	std::u8string text_;

public:
	Msg(int kind, int wparam, int lparam, std::u8string text)
		: kind_(kind)
		, wparam_(wparam)
		, lparam_(lparam)
		, text_(std::move(text))
	{
	}

	auto kind() const -> int {
		return kind_;
	}

	auto wparam() const -> int {
		return wparam_;
	}

	auto lparam() const -> int {
		return lparam_;
	}

	auto text() const -> std::u8string_view {
		return text_;
	}
};

// -----------------------------------------------
// サーバー
// -----------------------------------------------

static auto s_server = std::weak_ptr<KnowbugServerImpl>{};

class KnowbugServerImpl
	: public KnowbugServer
{
	// NOTE: メンバーの順番はデストラクタの呼び出し順序 (下から上へ) に影響する。

	HSP3DEBUG* debug_;

	HspObjects& objects_;

	HINSTANCE instance_;

	KnowbugStepController& step_controller_;

	bool started_;

	std::optional<WindowHandle> hidden_window_opt_;

	std::optional<KnowbugClientProcess> client_process_opt_;
	bool client_ready_;
	HWND client_hwnd_;
	std::u8string client_message_buf_;
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
	KnowbugServerImpl(HSP3DEBUG* debug, HspObjects& objects, HINSTANCE instance, KnowbugStepController& step_controller)
		: debug_(debug)
		, objects_(objects)
		, instance_(instance)
		, step_controller_(step_controller)
		, started_(false)
		, hidden_window_opt_()
		, client_process_opt_()
		, client_ready_(false)
		, client_hwnd_()
		, client_message_buf_()
		, pending_logmes_()
		, pending_runmode_(HSPDEBUG_RUN)
		, requested_mode_()
		, object_list_entity_(HspObjectListEntity::create())
	{
	}

	void start() override {
		if (std::exchange(started_, true)) {
			assert(false && u8"double start");
			return;
		}

		hidden_window_opt_ = create_hidden_window(instance_);

		client_process_opt_ = start_client_process(hidden_window_opt_->get());
		if (!client_process_opt_) {
			MessageBox(hidden_window_opt_->get(), TEXT("デバッグウィンドウの初期化に失敗しました。(クライアントプロセスを起動できません。)"), TEXT("knowbug"), MB_ICONERROR);
			return;
		}
	}

	void will_exit() override {
		send_terminated_event();
	}

	void logmes(HspStringView text) override {
		auto utf8_text = to_utf8(text);

		if (!client_ready_) {
			if (!pending_logmes_.empty()) {
				pending_logmes_ += u8"\r\n";
			}
			pending_logmes_ += utf8_text;
			return;
		}

		send_output_event(utf8_text);

		// (requested_mode_ の説明を参照)
		if (requested_mode_.has_value() && hidden_window_opt_) {
			PostMessage(hidden_window_opt_->get(), WM_APP, 0, 0);
		}
	}

	void debuggee_did_stop() override {
		if (!client_ready_) {
			pending_runmode_ = HSPDEBUG_STOP;
			return;
		}

		requested_mode_ = std::nullopt;

		send_stopped_event();
	}

	void handle_client_message(std::u8string_view text) {
		if (auto message_opt = knowbug_protocol_parse(text)){
			client_did_send_something(*message_opt);
		}
	}

	void client_did_send_something(KnowbugMessage const& message) {
		auto method = message.method();
		auto method_str = as_native(method);

		if (method == u8"initialize_notification") {
			auto client_hwnd = (HWND)(std::uintptr_t)message.get_int(u8"client_hwnd").value_or(0);
			client_did_initialize(client_hwnd);
			return;
		}

		if (method == u8"terminate_notification") {
			client_did_terminate();
			return;
		}

		if (method == u8"continue_notification") {
			client_did_step_continue();
			return;
		}

		if (method == u8"pause_notification") {
			client_did_step_pause();
			return;
		}

		if (method == u8"step_in_notification") {
			client_did_step_in();
			return;
		}

		if (method == u8"step_over_notification") {
			client_did_step_over();
			return;
		}

		if (method == u8"step_out_notification") {
			client_did_step_out();
			return;
		}

		if (method == u8"location_notification") {
			client_did_location_update();
			return;
		}

		if (method == u8"source_notification") {
			auto source_file_id = message.get_int(u8"source_file_id").value_or(0);
			client_did_source(source_file_id);
			return;
		}

		if (method == u8"list_update_notification") {
			client_did_list_update();
			return;
		}

		if (method == u8"list_toggle_expand_notification") {
			auto object_id = message.get_int(u8"object_id").value_or(0);
			client_did_list_toggle_expand(object_id);
			return;
		}

		if (method == u8"list_details_notification") {
			auto object_id = message.get_int(u8"object_id").value_or(0);
			client_did_list_details(object_id);
			return;
		}

		if (method.empty()) {
			return;
		}

		assert(false && u8"unknown method");
	}

	void client_did_initialize(HWND client_hwnd) {
		client_ready_ = true;
		client_hwnd_ = client_hwnd;
		send_initialized_event();

		// クライアントとの接続が確立する前にランタイムから受け取っていたイベントを伝える
		if (pending_runmode_ != HSPDEBUG_RUN) {
			send_stopped_event();
		}
		if (!pending_logmes_.empty()) {
			send_output_event(std::exchange(pending_logmes_, u8""));
		}
	}

	void client_did_terminate() {
		PostQuitMessage(EXIT_SUCCESS);
	}

	void client_did_step_continue() {
		hsx::debug_do_set_mode(HSPDEBUG_RUN, debug_);
		touch_all_windows();

		send_continued_event();
	}

	void client_did_step_pause() {
		requested_mode_ = (int)HSPDEBUG_STOP;
		hsx::debug_do_set_mode(HSPDEBUG_STOP, debug_);
		touch_all_windows();
	}

	void client_did_step_in() {
		hsx::debug_do_set_mode(HSPDEBUG_STEPIN, debug_);
		touch_all_windows();

		send_continued_event();
	}

	void client_did_step_over() {
		step_controller_.update(StepControl::new_step_over());
		touch_all_windows();

		send_continued_event();
	}

	void client_did_step_out() {
		step_controller_.update(StepControl::new_step_out());
		touch_all_windows();

		send_continued_event();
	}

	void client_did_location_update() {
		send_location_event();
	}

	void client_did_source(int source_file_id) {
		if (source_file_id < 0) {
			assert(false && u8"bad source_file_id");
			return;
		}

		send_source_event((std::size_t)source_file_id);
	}

	void client_did_list_update() {
		send_list_updated_events();
	}

	void client_did_list_toggle_expand(int object_id) {
		if (object_id < 0) {
			assert(false && u8"bad object_id");
			return;
		}

		object_list_entity_->toggle_expand((std::size_t)object_id);

		send_list_updated_events();
	}

	void client_did_list_details(int object_id) {
		if (object_id < 0) {
			assert(false && u8"bad object_id");
			return;
		}

		send_list_details_event((std::size_t)object_id);
	}

	void handle_after_logmes() {
		if (requested_mode_.has_value()) {
			hsx::debug_do_set_mode(requested_mode_.value(), debug_);
			touch_all_windows();
		}
	}

private:
	auto objects() -> HspObjects& {
		return objects_;
	}

	void send_message(KnowbugMessage const& message) {
		// この関数はクライアントからの初期化通知が来た後にのみ呼ばれる
		assert(client_ready_);

		if (!client_hwnd_) return;

		// クライアントが終了していたら送らない
		if (!client_process_opt_) return;

		debugf(u8"send_message '%s'", message.method().data());

		auto text = knowbug_protocol_serialize(message);
		auto copydata = COPYDATASTRUCT{};
		copydata.cbData = (DWORD)text.size();
		copydata.lpData = text.data();
		SendMessage(client_hwnd_, WM_COPYDATA, 0, (LPARAM)&copydata);
	}

	void send_message(std::u8string_view method) {
		auto message = KnowbugMessage::new_with_method(std::u8string{ method });
		send_message(message);
	}

	void send_initialized_event() {
		auto message = KnowbugMessage::new_with_method(std::u8string{ u8"initialized_event" });

		message.insert(std::u8string{ u8"version" }, std::u8string{ as_utf8(KNOWBUG_VERSION) });

		send_message(message);
	}

	void send_terminated_event() {
		send_message(u8"terminated_event");
	}

	void send_continued_event() {
		send_message(u8"continued_event");
	}

	void send_stopped_event() {
		send_message(u8"stopped_event");
	}

	void send_location_event() {
		objects().script_do_update_location();

		auto source_file_id = objects().script_to_current_file().value_or(0);
		auto line_index = objects().script_to_current_line();

		auto message = KnowbugMessage::new_with_method(std::u8string{ u8"location_event" });

		message.insert_int(std::u8string{ u8"source_file_id" }, (int)source_file_id);
		message.insert_int(std::u8string{ u8"line_index" }, (int)line_index);

		send_message(message);
	}

	void send_source_event(std::size_t source_file_id) {
		auto full_path_opt = objects().source_file_to_full_path(source_file_id);
		auto content_opt = objects().source_file_to_content(source_file_id);

		auto message = KnowbugMessage::new_with_method(std::u8string{ u8"source_event" });

		message.insert_int(std::u8string{ u8"source_file_id" }, (int)source_file_id);

		if (full_path_opt) {
			message.insert(std::u8string{ u8"source_path" }, std::u8string{ *full_path_opt });
		}

		if (content_opt) {
			message.insert(std::u8string{ u8"source_code" }, std::u8string{ *content_opt });
		}

		send_message(message);
	}

	void send_list_updated_events() {
		auto diff = object_list_entity_->update(objects());

		for (auto i = std::size_t{}; i < diff.size(); i++) {
			auto const& delta = diff[i];

			auto message = KnowbugMessage::new_with_method(std::u8string{ u8"list_updated_event" });

			message.insert(
				std::u8string{ u8"kind" },
				std::u8string{ delta.kind_name() }
			);

			message.insert_int(
				std::u8string{ u8"object_id" },
				(int)delta.object_id
			);

			message.insert_int(
				std::u8string{ u8"index" },
				(int)delta.index
			);

			message.insert(
				std::u8string{ u8"name" },
				delta.indented_name()
			);

			message.insert(
				std::u8string{ u8"value" },
				std::u8string{ delta.value }
			);

			if (delta.kind == HspObjectListDeltaKind::Remove && delta.count >= 2) {
				message.insert_int(
					std::u8string{ u8"count" },
					(int)delta.count
				);
			}

			send_message(message);
		}
	}

	void send_list_details_event(std::size_t object_id) {
		auto text_opt = std::optional<std::u8string>{};

		auto path_opt = object_list_entity_->object_id_to_path(object_id);
		if (path_opt) {
			auto string_writer = StringWriter{};
			HspObjectWriter{ objects(), string_writer }.write_table_form(**path_opt);
			text_opt = string_writer.finish();
		}

		auto message = KnowbugMessage::new_with_method(std::u8string{ u8"list_details_event" });

		message.insert_int(std::u8string{ u8"object_id" }, (int)object_id);

		if (text_opt) {
			message.insert(std::u8string{ u8"text" }, std::move(*text_opt));
		}

		send_message(message);
	}

	void send_output_event(std::u8string output) {
		auto message = KnowbugMessage::new_with_method(std::u8string{ u8"output_event" });

		message.insert(std::u8string{ u8"output" }, std::move(output));

		send_message(message);
	}

	void touch_all_windows() {
		auto hwnd = (HWND)debug_->hspctx->wnd_parent;
		if (!hwnd) {
			hwnd = HWND_BROADCAST;
		}

		// HACK: HSP のウィンドウに無意味なメッセージを送信することで、デバッグモードの変更に気づかせる。
		PostMessage(hwnd, WM_NULL, WPARAM{}, LPARAM{});
	}
};

auto KnowbugServer::create(HSP3DEBUG* debug, HspObjects& objects, HINSTANCE instance, KnowbugStepController& step_controller)->std::shared_ptr<KnowbugServer> {
	auto server = std::make_shared<KnowbugServerImpl>(debug, objects, instance, step_controller);
	s_server = server;
	return server;
}

// -----------------------------------------------
// グローバル
// -----------------------------------------------

static auto WINAPI process_hidden_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT {
	switch (msg) {
	case WM_CREATE:
		return TRUE;

	case WM_CLOSE:
		return FALSE;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_COPYDATA: {
		// データにクライアントからのメッセージが含まれている
		assert(lp);
		auto copydata = (COPYDATASTRUCT const *)lp;
		assert(copydata->cbData < (DWORD)INT32_MAX);
		auto text = std::u8string_view{(char8_t const*)copydata->lpData, copydata->cbData};

		if (auto server = s_server.lock()) {
			server->handle_client_message(text);
		}
		break;
	}
	case WM_APP:
	{
		// (このメッセージはサーバー自身によって送信される)
		if (auto server = s_server.lock()) {
			server->handle_after_logmes();
		}
		break;
	}
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}
