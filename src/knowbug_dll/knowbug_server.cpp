#include "pch.h"
#include <array>
#include <memory>
#include <deque>
#include <mutex>
#include <optional>
#include <vector>
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/platform.h"
#include "knowbug_app.h"
#include "knowbug_server.h"

static auto constexpr PIPE_BUFFER_SIZE = std::size_t{ 0x10000 };
static auto constexpr PIPE_MAX_INSTANCE_COUNT = std::size_t{ 1 };
static auto constexpr PIPE_TIMEOUT_MILLIS = std::size_t{ 50 };

// Knowbug window Message To the Server
#define KMTS_FIRST      (WM_USER + 1)
#define KMTS_HELLO      (WM_USER + 1)
#define KMTS_LAST       (WM_USER + 999)

// Knowbug window Message To the Client
#define KMTC_FIRST      (WM_USER + 1001)
#define KMTC_HELLO_OK   (WM_USER + 1001)
#define KMTC_SHUTDOWN   (WM_USER + 1002)
#define KMTC_LAST       (WM_USER + 1999)

class KnowbugServerImpl;

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

using PipeBuffer = std::array<Utf8Char, PIPE_BUFFER_SIZE>;

using PipeHandle = std::unique_ptr<HANDLE, Win32CloseHandleFn>;

using ProcessHandle = std::unique_ptr<HANDLE, Win32CloseHandleFn>;

using ThreadHandle = std::unique_ptr<HANDLE, Win32CloseHandleFn>;

using WindowHandle = std::unique_ptr<HWND, Win32DestroyWindowFn>;

class NamedPipeConnection {
	HANDLE pipe_handle_;

public:
	explicit NamedPipeConnection(PipeHandle const& pipe_handle)
		: pipe_handle_(pipe_handle.get())
	{
	}

	~NamedPipeConnection() {
		if (auto p = std::exchange(pipe_handle_, HANDLE{})) {
			DisconnectNamedPipe(p);
		}
	}

	auto pipe() const -> HANDLE {
		return pipe_handle_;
	}
};

static void fail_with(OsStringView reason) {
	MessageBox(HWND{}, to_owned(reason).data(), TEXT("knowbug"), MB_ICONWARNING);
	exit(EXIT_FAILURE);
}

// -----------------------------------------------
// パイプ
// -----------------------------------------------

static auto create_pipe(OsString const& pipe_name) -> PipeHandle {
	// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createnamedpipea
	auto pipe_handle = CreateNamedPipe(
		pipe_name.data(),
		// 双方向通信
		PIPE_ACCESS_DUPLEX,
		// PIPE_TYPE_MESSAGE: データをメッセージ単位で送受信する。
		PIPE_TYPE_MESSAGE,
		PIPE_MAX_INSTANCE_COUNT,
		PIPE_BUFFER_SIZE,
		PIPE_BUFFER_SIZE,
		PIPE_TIMEOUT_MILLIS,
		NULL
	);
	if (pipe_handle == INVALID_HANDLE_VALUE) {
		MessageBox(HWND{}, TEXT("デバッグウィンドウの初期化に失敗しました。(サーバーがパイプを作成できませんでした。)"), TEXT("knowbug"), MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	return PipeHandle{ pipe_handle };
}

static auto connect_pipe(PipeHandle const& pipe_handle) -> NamedPipeConnection {
	assert(pipe_handle.get());

	if (!ConnectNamedPipe(pipe_handle.get(), LPOVERLAPPED{})) {
		MessageBox(HWND{}, TEXT("デバッグウィンドウの初期化に失敗しました。(サーバーがパイプに接続できませんでした。)"), TEXT("knowbug"), MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	return NamedPipeConnection{ pipe_handle };
}

static void write_to_pipe(Utf8StringView text, PipeBuffer& buffer, NamedPipeConnection const& connection) {
	assert(buffer.size() >= PIPE_BUFFER_SIZE);
	if (text.size() >= PIPE_BUFFER_SIZE) {
		assert(false && u8"too long to write");
		// FIXME: ログ出力
		return;
	}

	auto data = text.data();
	auto size = text.size();

	if (data[size] != Utf8Char{}) {
		// NULL 文字で終端されていない場合はバッファーにコピーして送信する。
		std::memcpy(buffer.data(), data, size);
		buffer[size] = Utf8Char{};
	}
	assert(data[size] == Utf8Char{});

	auto written_size = DWORD{};
	auto success = WriteFile(connection.pipe(), data, (DWORD)size, &written_size, LPOVERLAPPED{});
	if (!success) {
		assert(false && u8"failed to write to the pipe");
		// FIXME: ログ出力
		return;
	}
	assert(std::size_t{ written_size } == size);
}

// パイプからデータを読み、それを UTF-8 エンコーディングされた文字列として解釈したものを返す。
// 結果は渡したバッファーへの参照である。
static auto read_from_pipe(std::size_t expected_size, PipeBuffer& buffer, NamedPipeConnection const& connection) -> Utf8StringView {
	assert(buffer.size() >= PIPE_BUFFER_SIZE);
	if (expected_size >= PIPE_BUFFER_SIZE) {
		assert(false && u8"too long to read");
		// FIXME: ログ出力
		return as_utf8(u8"");
	}

	auto actual_size = DWORD{};
	auto success = ReadFile(connection.pipe(), buffer.data(), (DWORD)expected_size, &actual_size, LPOVERLAPPED{});
	if (!success) {
		assert(false && u8"failed to read from the pipe");
		return as_utf8(u8"");
	}
	assert(actual_size == expected_size);

	auto size = std::min(std::size_t{ actual_size }, expected_size);
	buffer.data()[size] = Utf8Char{};

	return Utf8StringView{ buffer.data(), size };
}

// -----------------------------------------------
// 隠しウィンドウ
// -----------------------------------------------

static auto WINAPI process_hidden_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT;

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

	auto size_x = 100;
	auto size_y = 100;
	auto pos_x = -size_x;
	auto pos_y = -size_y;
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
		LPARAM{}
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

// FIXME: knowbug_config と重複
static auto get_hsp_dir() -> OsString {
	// DLL の絶対パスを取得する。
	auto buffer = std::array<TCHAR, MAX_PATH>{};
	GetModuleFileName(GetModuleHandle(nullptr), buffer.data(), buffer.size());
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

static auto start_client_process(HWND server_hwnd) -> std::pair<ThreadHandle, ProcessHandle> {
	auto name = get_hsp_dir();
	name += TEXT("knowbug_client.exe");

	auto cmdline = to_os(ascii_as_utf8(std::to_string((UINT_PTR)server_hwnd)));

	auto si = STARTUPINFO{ sizeof(STARTUPINFO) };
	auto pi = PROCESS_INFORMATION{};

	auto success = CreateProcess(
		name.data(),
		cmdline.data(),
		LPSECURITY_ATTRIBUTES{},
		LPSECURITY_ATTRIBUTES{},
		FALSE,
		NORMAL_PRIORITY_CLASS,
		LPTSTR{},
		LPCTSTR{},
		&si,
		&pi
	);
	if (!success) {
		MessageBox(HWND{}, TEXT("デバッグウィンドウの初期化に失敗しました。(クライアントプロセスを起動できませんでした。)"), TEXT("knowbug"), MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	auto thread_handle = ThreadHandle{ pi.hThread };
	auto process_handle = ProcessHandle{ pi.hProcess };
	return std::make_pair(std::move(thread_handle), std::move(process_handle));
}

// -----------------------------------------------
// サーバー
// -----------------------------------------------

static auto s_server = std::weak_ptr<KnowbugServerImpl>{};

class KnowbugServerImpl
	: public KnowbugServer
{
	// NOTE: メンバーの順番はデストラクタの呼び出し順序 (下から上へ) に影響する。

	HSP3DEBUG* debug_;

	HINSTANCE instance_;

	bool started_;

	OsString pipe_name_;

	PipeBuffer pipe_buffer_;

	std::optional<PipeHandle> pipe_opt_;

	std::optional<NamedPipeConnection> pipe_connection_opt_;

	std::optional<WindowHandle> hidden_window_opt_;

	std::optional<HWND> client_hwnd_opt_;

	std::optional<ProcessHandle> client_process_opt_;

	std::optional<ThreadHandle> client_thread_opt_;

public:
	KnowbugServerImpl(HSP3DEBUG* debug, HINSTANCE instance)
		: debug_(debug)
		, instance_(instance)
		, started_(false)
		, hidden_window_opt_()
		, pipe_name_(TEXT("\\\\.\\pipe\\knowbug"))
		, pipe_buffer_()
		, pipe_opt_()
		, pipe_connection_opt_()
		, client_hwnd_opt_()
		, client_process_opt_()
		, client_thread_opt_()
	{
	}

	void start() {
		if (std::exchange(started_, true)) {
			assert(false && u8"double start");
			return;
		}

		pipe_opt_ = create_pipe(pipe_name_);
		pipe_connection_opt_ = connect_pipe(*pipe_opt_);

		hidden_window_opt_ = create_hidden_window(instance_);

		auto pair = start_client_process(hidden_window_opt_->get());
		client_thread_opt_ = std::move(pair.first);
		client_process_opt_ = std::move(pair.second);
	}

	void will_exit() override {
		send(KMTC_SHUTDOWN);
	}

	void client_did_hello(HWND client_hwnd) {
		if (!client_hwnd) {
			fail_with(TEXT("The client sent invalid hwnd!"));
		}

		assert(!client_hwnd_opt_);
		client_hwnd_opt_ = client_hwnd;

		send(KMTC_HELLO_OK);
	}

private:
	void send(int kind, Utf8StringView text) {
		if (!client_hwnd_opt_ || !pipe_connection_opt_) {
			// FIXME: バッファーか何かに溜めて後で送る
			return;
		}

		write_to_pipe(text, pipe_buffer_, *pipe_connection_opt_);
		SendMessage(*client_hwnd_opt_, kind, WPARAM{}, text.size());
	}

	void send(int kind) {
		send(kind, Utf8StringView{});
	}
};

auto KnowbugServer::start(HSP3DEBUG* debug, HINSTANCE instance)->std::shared_ptr<KnowbugServer> {
	auto server = std::make_shared<KnowbugServerImpl>(debug, instance);
	s_server = server;
	server->start();
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

	default:
		if (KMTS_FIRST <= msg && msg <= KMTS_LAST) {
			if (auto server = s_server.lock()) {
				switch (msg) {
				case KMTS_HELLO:
					server->client_did_hello((HWND)lp);
					break;

				default:
					assert(false && u8"unknown msg from the client");
					break;
				}
			}
			return TRUE;
		}
		break;
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}
