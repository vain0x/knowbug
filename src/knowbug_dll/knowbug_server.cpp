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

static auto constexpr MEMORY_BUFFER_SIZE = std::size_t{ 0x10000 };

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

class Win32UnmapViewOfFileFn {
public:
	using pointer = LPVOID;

	void operator()(LPVOID p) {
		UnmapViewOfFile(p);
	}
};

using MemoryMappedFile = std::unique_ptr<HANDLE, Win32CloseHandleFn>;

using MemoryMappedFileView = std::unique_ptr<LPVOID, Win32UnmapViewOfFileFn>;

using ProcessHandle = std::unique_ptr<HANDLE, Win32CloseHandleFn>;

using ThreadHandle = std::unique_ptr<HANDLE, Win32CloseHandleFn>;

using WindowHandle = std::unique_ptr<HWND, Win32DestroyWindowFn>;

static void fail_with(OsStringView reason) {
	MessageBox(HWND{}, to_owned(reason).data(), TEXT("knowbug"), MB_ICONWARNING);
	exit(EXIT_FAILURE);
}

// -----------------------------------------------
// メモリマップドファイル
// -----------------------------------------------

static auto create_memory_mapped_file(OsString const& name) -> MemoryMappedFile {
	auto mapping_handle = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		LPSECURITY_ATTRIBUTES{},
		PAGE_READWRITE,
		DWORD{},
		(DWORD)MEMORY_BUFFER_SIZE,
		name.data()
	);
	if (!mapping_handle) {
		MessageBox(HWND{}, TEXT("デバッグウィンドウの初期化に失敗しました。(サーバーがデータ交換バッファーを作成できませんでした。)"), TEXT("knowbug"), MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	return MemoryMappedFile{ mapping_handle };
}

static auto connect_memory_mapped_file(MemoryMappedFile const& mapping_handle) -> MemoryMappedFileView {
	auto data = MapViewOfFile(mapping_handle.get(), FILE_MAP_ALL_ACCESS, 0, 0, MEMORY_BUFFER_SIZE);
	if (!data) {
		MessageBox(HWND{}, TEXT("デバッグウィンドウの初期化に失敗しました。(サーバーがデータ交換バッファーへのビューを作成できませんでした。)"), TEXT("knowbug"), MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	return MemoryMappedFileView{ data };
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

	auto server_hwnd_str = to_os(as_utf8(std::to_string((UINT_PTR)server_hwnd)));

	auto cmdline = OsString{ TEXT("\"") };
	cmdline += name;
	cmdline += TEXT("\" ");
	cmdline += server_hwnd_str;

	auto si = STARTUPINFO{ sizeof(STARTUPINFO) };
	auto pi = PROCESS_INFORMATION{};

	auto success = CreateProcess(
		LPCTSTR{},
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

	OsString server_buffer_name_;

	std::optional<MemoryMappedFile> server_buffer_opt_;

	std::optional<MemoryMappedFileView> server_buffer_view_opt_;

	OsString client_buffer_name_;

	std::optional<MemoryMappedFile> client_buffer_opt_;

	std::optional<MemoryMappedFileView> client_buffer_view_opt_;

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
		, server_buffer_name_(TEXT("KnowbugServerBuffer"))
		, server_buffer_opt_()
		, server_buffer_view_opt_()
		, client_buffer_name_(TEXT("KnowbugClientBuffer"))
		, client_buffer_opt_()
		, client_buffer_view_opt_()
		, client_hwnd_opt_()
		, client_process_opt_()
		, client_thread_opt_()
	{
	}

	void start() override {
		if (std::exchange(started_, true)) {
			assert(false && u8"double start");
			return;
		}

		hidden_window_opt_ = create_hidden_window(instance_);

		//server_buffer_opt_ = create_memory_mapped_file(server_buffer_name_);

		//server_buffer_view_opt_ = connect_memory_mapped_file(*server_buffer_opt_);

		//client_buffer_opt_ = create_memory_mapped_file(client_buffer_name_);

		//client_buffer_view_opt_ = connect_memory_mapped_file(*client_buffer_opt_);

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
		//if (!client_hwnd_opt_ || !server_buffer_view_opt_) {
		if (!client_hwnd_opt_) {
			// FIXME: キューに溜めて後で送る
			return;
		}

		// write to the buffer
		SendMessage(*client_hwnd_opt_, kind, WPARAM{}, text.size());
	}

	void send(int kind) {
		send(kind, Utf8StringView{});
	}
};

auto KnowbugServer::create(HSP3DEBUG* debug, HINSTANCE instance)->std::shared_ptr<KnowbugServer> {
	auto server = std::make_shared<KnowbugServerImpl>(debug, instance);
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
