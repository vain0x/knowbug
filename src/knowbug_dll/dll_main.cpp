#include "pch.h"

#include "../knowbug_core/encoding.h"
#include "./logger.h"

// NOTE: このソースファイルに knowbug の Windows プログラミング的な部分が集約されている。
//       ほかのソースファイルと異なり、Windows プログラミングらしいスタイルでコードが書かれている

// このDLLのインスタンスハンドル
static HINSTANCE s_instance;

// サーバーウィンドウ (knowbug のメッセージ専用ウィンドウ) のハンドル
static HWND s_server_hwnd;
static HSP3DEBUG* debugctx;

struct KnowbugClientProcess {
	HANDLE hThread;
	HANDLE hProcess;
};

static KnowbugClientProcess s_client_process;

// ウィンドウクラス名 (適当な名前)
static constexpr LPCTSTR KnowbugServerWndClass = TEXT("KnowbugHiddenWindowClass");

// ウィンドウプロシージャ
static LRESULT WINAPI KnowbugServerWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

// knowbug のサーバーウィンドウを作成する
static HWND create_server_window(HINSTANCE hInstance) {
	WNDCLASS wc = {};
	wc.lpfnWndProc = KnowbugServerWndProc;
	wc.hInstance = hInstance;
	//wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszClassName = KnowbugServerWndClass;
	RegisterClass(&wc);

	auto hwnd = CreateWindow(
		KnowbugServerWndClass,
		// タイトル
		TEXT("Knowbug Hidden Window"),
		// ウィンドウスタイル
		0,
		// 位置・大きさ
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		// 親ウィンドウ
		// ここに特殊な定数を与えることで、このウィンドウをメッセージ専用ウィンドウとして指定する
		// c.f. https://learn.microsoft.com/ja-jp/windows/win32/winmsg/window-features#message-only-windows
		HWND_MESSAGE,
		// メニュー
		NULL,
		hInstance,
		// lparam
		NULL
	);
	if (!hwnd) {
		MessageBox(NULL, TEXT("デバッグウィンドウの初期化に失敗しました。(サーバーのメッセージウィンドウの作成に失敗しました)"), TEXT("knowbug"), MB_ICONERROR);
		exit(EXIT_FAILURE);
	}
	return hwnd;
}

static auto create_client_process(OsString hsp_dir, HWND server_hwnd, HINSTANCE hInstance) -> KnowbugClientProcess {
	// クライアントプロセスの起動コマンドラインを構成する
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

	STARTUPINFO si = {};
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION pi = {};

	// クライアントプロセスを起動する。
	if (!CreateProcess(
		// lpApplicationName (コマンドラインを指定するので使用しない)
		NULL,
		cmdline.data(),
		// プロセス・スレッドの属性 (使用しない)
		NULL,
		NULL,
		// ハンドルを継承する (bInheritHandles)
		TRUE,
		NORMAL_PRIORITY_CLASS,
		// 環境変数 (使用しない)
		NULL,
		// カレントディレクトリ
		hsp_dir.data(),
		&si,
		&pi
	)) {
		auto err = GetLastError();
		TCHAR text[256] = {};
		_stprintf_s(text, TEXT("knowbug クライアントの起動に失敗しました (err = %d)"), err);
		MessageBox(NULL, text, TEXT("knowbug"), MB_ICONERROR);
		exit(EXIT_FAILURE);
	}

	KnowbugClientProcess cp = {};
	cp.hProcess = pi.hProcess;
	cp.hThread = pi.hThread;
	return cp;
}

EXPORT BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH: {
		// この部分はデバッグウィンドウの初期化前に呼ばれる

		s_instance = hInstance;

		// プロセスにアタッチするタイミングを与えるため、一時停止する
#if _DEBUG
		if ((GetKeyState(VK_SHIFT) & 0x8000) != 0 || (GetKeyState(VK_CONTROL) & 0x8000) != 0) {
			MessageBox(NULL, TEXT("knowbug の実行が一時停止しました。Visual Studio でプロセス hsp3.exe にアタッチし、デバッグを開始できます (Ctrl+Alt+P)"), TEXT("knowbug"), MB_OK);
		}
#endif
		break;
	}
	case DLL_PROCESS_DETACH: {
		// この部分はデバッグの終了時に呼ばれる

		// クライアントに終了を通知する
		//app->will_exit();
		// server().will_exit();
		// send_terminated_event();
		//g_app.reset();

		// (~KnowbugServer())
		//CloseHandle(s_client_process.hProcess); s_client_process.hProcess = NULL;
		//CloseHandle(s_client_process.hThread); s_client_process.hThread = NULL;
		//DestroyWindow(s_server_hwnd); s_server_hwnd = NULL;
		break;
	}
	}
	return TRUE;
}

EXPORT BOOL WINAPI debugini(HSP3DEBUG* p1, int p2, int p3, int p4) {
	// この関数はデバッグウィンドウの初期化のために呼ばれる

	assert(s_instance);

	debugctx = p1;
	ctx = p1->hspctx;
	exinfo = p1->hspctx->exinfo2;

	OsString hsp_dir;
	{
		// DLL の絶対パスを取得する。
		TCHAR buffer[MAX_PATH] = {};
		GetModuleFileName(GetModuleHandle(NULL), buffer, sizeof(buffer) / sizeof(TCHAR));
		auto full_path = OsString{ buffer };

		// ファイル名の部分を削除
		while (!full_path.empty()) {
			auto last = full_path[full_path.length() - 1];
			if (last == TEXT('/') || last == TEXT('\\')) {
				break;
			}

			full_path.pop_back();
		}
		hsp_dir = full_path;
	}
	debugf(u8"hsp_dir = %s", to_utf8(hsp_dir));

	OsString common_dir = hsp_dir;
	common_dir += TEXT("\\common\\");

	// hspctxにあるデバッグ情報を解析
	// デバッグ情報とファイルシステムを使ってソースファイル情報を解析
	// 変数リスト、モジュールツリーなどの静的データを解析
	//auto resolver = SourceFileResolver{ g_fs };
	//auto objects_builder = HspObjectsBuilder{};
	//resolver.add_known_dir(std::move(common_dir));
	//objects_builder.read_debug_segment(resolver, ctx);
	//auto source_file_repository = std::make_unique<SourceFileRepository>(resolver.resolve());
	//auto objects = std::make_unique<HspObjects>(objects_builder.finish(debug, std::move(source_file_repository)));

	//g_app = std::make_shared<KnowbugAppImpl>(
	//	std::move(step_controller),
	//	std::move(objects)
	//);

	//// 起動処理:

	//if (auto app = std::shared_ptr{ g_app }) {
	//	app->initialize();
	//
		// WrapCallモジュールに HSP3DEBUG* が渡される
		//objects().initialize();
		// wc_initialize(wc_debugger_);

		// サーバーのメッセージウィンドウの作成
		// クライアントプロセスの生成
		// (実行ファイルパスの計算にhsp_dirを使う)
		// ここでクライアントプロセスの初期化が完了するのを待つことはできない (おそらく)
		// (ここでメインスレッドをブロックしてしまうと、メッセージの受け取りがまったくできず、
		//  クライアントから送られてくる初期化完了のメッセージも受け取れないため、初期化が完了しないことになってしまう)
		//server().start();
		// 
		//if (std::exchange(started_, true)) {
		//	assert(false && u8"double start");
		//	return;
		//}

		//hidden_window_opt_ = create_hidden_window(instance_);
	s_server_hwnd = create_server_window(s_instance);
	assert(s_server_hwnd);

	//client_process_opt_ = start_client_process(hidden_window_opt_->get());
	//if (!client_process_opt_) {
	//	MessageBox(hidden_window_opt_->get(), TEXT("デバッグウィンドウの初期化に失敗しました。(クライアントプロセスを起動できません。)"), TEXT("knowbug"), MB_ICONERROR);
	//	return;
	//}
	// 
	s_client_process = create_client_process(hsp_dir, s_server_hwnd, s_instance);
	//}
	return 0;
}

EXPORT BOOL WINAPI debug_notice(HSP3DEBUG* p1, int p2, int p3, int p4) {
	switch (p2) {
	case 0:
		// この部分は、assert/stop 命令や、ステップインなどによって実行が停止したときに呼ばれる

		//app->did_hsp_pause();

		// いま step-over/step-out の実行中で、脱出条件である sublev より深ければ、自動で stepin を行う
		//if (step_controller_->continue_step_running()) {
		//	// HACK: すべてのウィンドウに無意味なメッセージを送信する。
		//	//       HSP のウィンドウがこれを受信したとき、デバッグモードの変化が再検査されて、
		//	//       ステップ実行モードが変化したことに気づいてくれる (実装依存)。
		//	PostMessage(HWND_BROADCAST, WM_NULL, 0, 0);
		//	return;
		//}

		// (クライアントの初期化が完了したら) 一時停止状態をクライアントに通知する
		// (この時点でサーバーはアクションしない。この後クライアントから現在情報の問い合わせが来る)
		//server().debuggee_did_stop();
		//if (!client_ready_) {
		//	pending_runmode_ = HSPDEBUG_STOP;
		//	return;
		//}
		//requested_mode_ = std::nullopt;
		//send_stopped_event();
		break;

	case 1:
		// この部分は、logmes 命令が実行されたときに呼ばれる
		// (`ctx->stmp` に指定された文字列が含まれている)
		//app->did_hsp_logmes(as_hsp(ctx->stmp));

		// (クライアントの初期化が完了したら) ログの追加をクライアントに通知する
		//server().logmes(text);
		//auto utf8_text = to_utf8(text);
		//if (!client_ready_) {
		//	if (!pending_logmes_.empty()) {
		//		pending_logmes_ += u8"\r\n";
		//	}
		//	pending_logmes_ += utf8_text;
		//	return;
		//}
		//send_output_event(utf8_text);
		//if (requested_mode_.has_value() && hidden_window_opt_) {
		//	PostMessage(hidden_window_opt_->get(), WM_APP, 0, 0);
		//}

		// HspObjectsの内部データにも同じ変更を加える
		//objects().log_do_append(to_utf8(text));
		//objects().log_do_append(u8"\r\n");
		break;
	}
	return 0;
}

// knowbug サーバーウィンドウのウィンドウプロシージャ
LRESULT WINAPI KnowbugServerWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
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
		auto copydata = (const COPYDATASTRUCT*)lp;
		assert(copydata->cbData < (DWORD)INT32_MAX);
		auto text = std::u8string_view{ (const char8_t*)copydata->lpData, copydata->cbData };

		// メッセージをパースして、ディスパッチする (メソッドごとの処理を行う)
		//if (auto server = s_server.lock()) {
		//	server->handle_client_message(text);
		//}
		//if (auto message_opt = knowbug_protocol_parse(text)){
		//	client_did_send_something(*message_opt);
		//}
		break;
	}
	case WM_APP: {
		// (pending な非同期処理をpollする)
		//if (auto server = s_server.lock()) {
		//	server->handle_after_logmes();
		//}
		break;
	}
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}
