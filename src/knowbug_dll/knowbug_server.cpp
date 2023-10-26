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
#include "knowbug_app.h"
#include "knowbug_server.h"
#include "logger.h"

class KnowbugServerImpl;

static constexpr auto MEMORY_BUFFER_SIZE = std::size_t{ 1024 * 1024 };

// -----------------------------------------------
// バージョン
// -----------------------------------------------

static constexpr auto KNOWBUG_VERSION = u8"v2.2.0";

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
// オブジェクトリスト
// -----------------------------------------------

class HspObjectIdProvider {
public:
	virtual auto path_to_object_id(HspObjectPath const& path)->std::size_t = 0;

	virtual auto object_id_to_path(std::size_t object_id)->std::optional<std::shared_ptr<HspObjectPath const>> = 0;
};

class HspObjectListExpansion {
public:
	virtual auto is_expanded(HspObjectPath const& path) const -> bool = 0;
};

class HspObjectListItem {
	std::size_t object_id_;
	std::size_t depth_;
	std::u8string name_;
	std::u8string value_;
	std::size_t child_count_;

public:
	HspObjectListItem(std::size_t object_id, std::size_t depth, std::u8string name, std::u8string value, std::size_t child_count)
		: object_id_(object_id)
		, depth_(depth)
		, name_(std::move(name))
		, value_(std::move(value))
		, child_count_(child_count)
	{
	}

	auto object_id() const -> std::size_t {
		return object_id_;
	}

	auto depth() const ->std::size_t {
		return depth_;
	}

	auto name() const ->std::u8string_view {
		return name_;
	}

	auto value() const ->std::u8string_view {
		return value_;
	}

	auto child_count() const -> std::size_t {
		return child_count_;
	}

	auto equals(HspObjectListItem const& other) const -> bool {
		return object_id() == other.object_id()
			&& depth() == other.depth()
			&& name() == other.name()
			&& value() == other.value()
			&& child_count() == other.child_count();
	}
};

class HspObjectList {
	std::vector<HspObjectListItem> items_;

public:
	auto items() const ->std::vector<HspObjectListItem> const& {
		return items_;
	}

	auto size() const -> std::size_t {
		return items().size();
	}

	auto operator[](std::size_t index) const -> HspObjectListItem const& {
		return items().at(index);
	}

	auto find_by_object_id(std::size_t object_id) const -> std::optional<HspObjectListItem const*> {
		for (auto&& item : items()) {
			if (item.object_id() == object_id) {
				return &item;
			}
		}
		return std::nullopt;
	}

	void add_item(HspObjectListItem item) {
		items_.push_back(std::move(item));
	}
};

// オブジェクトリストを構築する関数。
class HspObjectListWriter {
	HspObjects& objects_;
	HspObjectList& object_list_;
	HspObjectIdProvider& id_provider_;
	HspObjectListExpansion& expansion_;

	std::size_t depth_;

public:
	HspObjectListWriter(HspObjects& objects, HspObjectList& object_list, HspObjectIdProvider& id_provider, HspObjectListExpansion& expansion)
		: objects_(objects)
		, object_list_(object_list)
		, id_provider_(id_provider)
		, expansion_(expansion)
		, depth_()
	{
	}

	void add(HspObjectPath const& path) {
		if (path.kind() == HspObjectKind::Ellipsis) {
			add_value(path, path);
			return;
		}

		if (path.visual_child_count(objects()) == 1) {
			auto value_path_opt = path.visual_child_at(0, objects());
			assert(value_path_opt);

			if (value_path_opt) {
				switch ((**value_path_opt).kind()) {
				case HspObjectKind::Label:
				case HspObjectKind::Str:
				case HspObjectKind::Double:
				case HspObjectKind::Int:
				case HspObjectKind::Unknown:
					add_value(path, **value_path_opt);
					return;

				default:
					break;
				}
			}
		}

		add_scope(path);
	}

	void add_children(HspObjectPath const& path) {
		if (!expansion_.is_expanded(path)) {
			return;
		}

		auto item_count = path.visual_child_count(objects());
		for (auto i = std::size_t{}; i < item_count; i++) {
			auto item_path_opt = path.visual_child_at(i, objects());
			if (!item_path_opt) {
				assert(false);
				continue;
			}

			add(**item_path_opt);
		}
	}

private:
	void add_scope(HspObjectPath const& path) {
		auto name = path.name(objects());
		auto item_count = path.visual_child_count(objects());

		auto value = std::u8string{ u8"(" };
		value += as_utf8(std::to_string(item_count));
		value += u8"):";

		auto object_id = id_provider_.path_to_object_id(path);
		object_list_.add_item(HspObjectListItem{ object_id, depth_, name, value, item_count });
		depth_++;
		add_children(path);
		depth_--;
	}

	void add_value(HspObjectPath const& path, HspObjectPath const& value_path) {
		auto name = path.name(objects());

		auto value_writer = StringWriter{};
		HspObjectWriter{ objects(), value_writer }.write_flow_form(value_path);
		auto value = value_writer.finish();

		auto object_id = id_provider_.path_to_object_id(path);
		object_list_.add_item(HspObjectListItem{ object_id, depth_, name, value, 0 });
	}

	auto objects() -> HspObjects& {
		return objects_;
	}
};

class HspObjectListDelta {
public:
	enum class Kind {
		Insert,
		Remove,
		Update,
	};

	static auto kind_to_string(Kind kind) -> std::u8string_view {
		switch (kind) {
		case Kind::Insert:
			return u8"insert";

		case Kind::Remove:
			return u8"remove";

		case Kind::Update:
			return u8"update";

		default:
			throw std::exception{};
		}
	}

private:
	Kind kind_;
	std::size_t object_id_;
	std::size_t index_;
	std::size_t count_;
	std::size_t depth_;
	std::u8string name_;
	std::u8string value_;

public:
	HspObjectListDelta(Kind kind, std::size_t object_id, std::size_t index, std::size_t depth, std::u8string name, std::u8string value)
		: kind_(kind)
		, object_id_(object_id)
		, index_(index)
		, count_(0)
		, depth_(depth)
		, name_(std::move(name))
		, value_(std::move(value))
	{
	}

	static auto new_insert(std::size_t index, HspObjectListItem const& item) -> HspObjectListDelta {
		return HspObjectListDelta{
			Kind::Insert,
			item.object_id(),
			index,
			item.depth(),
			std::u8string{ item.name() },
			std::u8string{ item.value() }
		};
	}

	static auto new_remove(std::size_t object_id, std::size_t index, std::size_t count) -> HspObjectListDelta {
		assert(count >= 1);
		auto delta = HspObjectListDelta{
			Kind::Remove,
			object_id,
			index,
			std::size_t{},
			std::u8string{},
			std::u8string{}
		};
		delta.count_ = count;
		return delta;
	}

	static auto new_update(std::size_t index, HspObjectListItem const& item) -> HspObjectListDelta {
		return HspObjectListDelta{
			Kind::Update,
			item.object_id(),
			index,
			item.depth(),
			std::u8string{ item.name() },
			std::u8string{ item.value() }
		};
	}

	auto kind() const -> Kind {
		return kind_;
	}

	auto object_id() const -> std::size_t {
		return object_id_;
	}

	auto index() const -> std::size_t {
		return index_;
	}

	auto count() const -> std::size_t {
		assert(kind() == Kind::Remove);
		return count_;
	}

	auto name() const -> std::u8string {
		static constexpr auto SPACES = u8"                ";

		auto name = std::u8string{ as_utf8(SPACES).substr(0, depth_ * 2) };
		name += name_;
		return name;
	}

	auto value() const -> std::u8string_view {
		return value_;
	}

	auto with_count(std::size_t count) ->HspObjectListDelta {
		return HspObjectListDelta::new_remove(object_id(), index(), count);
	}
};

static auto diff_object_list(HspObjectList const& source, HspObjectList const& target, std::vector<HspObjectListDelta>& diff) {
	auto source_done = std::vector<bool>{};
	source_done.resize(source.size());

	auto target_done = std::vector<bool>{};
	target_done.resize(target.size());

	auto push_remove = [&](std::size_t object_id, std::size_t index) {
		if (!diff.empty()) {
			auto& last = diff.back();
			if (last.kind() == HspObjectListDelta::Kind::Remove && last.index() == index) {
				last = last.with_count(last.count() + 1);
				return;
			}
		}

		diff.push_back(HspObjectListDelta::new_remove(object_id, index, 1));
	};

	// FIXME: 高速化
	for (auto si = std::size_t{}; si < source.size(); si++) {
		if (source_done[si]) {
			continue;
		}

		for (auto ti = std::size_t{}; ti < target.size(); ti++) {
			if (target_done[ti]) {
				continue;
			}

			if (source[si].object_id() == target[ti].object_id()) {
				source_done[si] = true;
				target_done[ti] = true;
				break;
			}
		}
	}

	{
		auto si = std::size_t{};
		auto ti = std::size_t{};

		while (si < source.size() || ti < target.size()) {
			if (ti == target.size() || (si < source.size() && !source_done[si])) {
				push_remove(source[si].object_id(), ti);
				si++;
				continue;
			}

			if (si == source.size() || (ti < target.size() && !target_done[ti])) {
				diff.push_back(HspObjectListDelta::new_insert(ti, target[ti]));
				ti++;
				continue;
			}

			assert(si < source.size() && ti < target.size());
			assert(source_done[si] && target_done[ti]);

			if (source[si].object_id() == target[ti].object_id()) {
				auto const& s = source[si];
				auto const& t = target[ti];
				if (!s.equals(t)) {
					diff.push_back(HspObjectListDelta::new_update(ti, target[ti]));
				}

				si++;
				ti++;
				continue;
			}

			assert(false && u8"パスの順番が入れ替わるケースは未実装。");
			diff.clear();
			break;
		}
	}
}

class HspObjectListEntity
	: public HspObjectIdProvider
	, public HspObjectListExpansion
{
	HspObjectList object_list_;

	std::size_t last_id_;
	std::unordered_map<std::size_t, std::shared_ptr<HspObjectPath const>> id_to_paths_;
	std::unordered_map<std::shared_ptr<HspObjectPath const>, std::size_t> path_to_ids_;

	std::unordered_map<std::shared_ptr<HspObjectPath const>, bool> expanded_;

public:
	HspObjectListEntity()
		: object_list_()
		, last_id_()
		, id_to_paths_()
		, path_to_ids_()
		, expanded_()
	{
	}

	auto size() const -> std::size_t {
		return object_list_.size();
	}

	auto path_to_object_id(HspObjectPath const& path) -> std::size_t override {
		auto iter = path_to_ids_.find(path.self());
		if (iter == path_to_ids_.end()) {
			auto id = ++last_id_;
			path_to_ids_[path.self()] = id;
			id_to_paths_[id] = path.self();
			return id;
		}

		return iter->second;
	}

	auto object_id_to_path(std::size_t object_id) -> std::optional<std::shared_ptr<HspObjectPath const>> override {
		auto iter = id_to_paths_.find(object_id);
		if (iter == id_to_paths_.end()) {
			return std::nullopt;
		}

		return iter->second;
	}

	auto is_expanded(HspObjectPath const& path) const -> bool {
		auto iter = expanded_.find(path.self());
		if (iter == expanded_.end()) {
			// ルートの子要素は既定で開く。
			return path.parent().kind() == HspObjectKind::Root;
		}

		return iter->second;
	}

	auto update(HspObjects& objects) -> std::vector<HspObjectListDelta> {
		auto new_list = HspObjectList{};
		HspObjectListWriter{ objects, new_list, *this, *this }.add_children(objects.root_path());

		auto diff = std::vector<HspObjectListDelta>{};
		diff_object_list(object_list_, new_list, diff);

		for (auto&& delta : diff) {
			apply_delta(delta, new_list);
		}

		object_list_ = std::move(new_list);
		return diff;
	}

	void toggle_expand(std::size_t object_id) {
		auto path_opt = object_id_to_path(object_id);
		if (!path_opt) {
			return;
		}

		auto item_opt = object_list_.find_by_object_id(object_id);
		if (!item_opt) {
			return;
		}

		// 子要素のないノードは開閉しない。
		if ((**item_opt).child_count() == 0) {
			return;
		}

		expanded_[*path_opt] = !is_expanded(**path_opt);
	}

	auto expand(std::size_t object_id, bool expand) {
		auto path_opt = object_id_to_path(object_id);
		if (!path_opt) {
			return;
		}

		if (is_expanded(**path_opt) != expand) {
			toggle_expand(object_id);
		}
	}

private:
	void apply_delta(HspObjectListDelta const& delta, HspObjectList& new_list) {
		switch (delta.kind()) {
		case HspObjectListDelta::Kind::Remove: {
			auto object_id = delta.object_id();
			auto iter = id_to_paths_.find(object_id);
			if (iter == id_to_paths_.end()) {
				assert(false);
				return;
			}

			auto path = iter->second;
			id_to_paths_.erase(iter);

			{
				auto iter = path_to_ids_.find(path);
				if (iter != path_to_ids_.end()) {
					path_to_ids_.erase(iter);
				}
			}

			{
				auto iter = expanded_.find(path);
				if (iter != expanded_.end()) {
					expanded_.erase(iter);
				}
			}
			return;
		}
		default:
			return;
		}
	}
};

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
// 名前付きパイプ
// -----------------------------------------------

static constexpr auto PIPE_BUFFER_SIZE = DWORD{ 0x8000 };

// 名前付きパイプの名前を生成する
static auto compute_pipe_name(char const* prefix) -> OsString {
	auto pid = GetCurrentProcessId();

	char buf[128];
	auto len = sprintf_s(buf, "\\\\.\\pipe\\LOCAL\\KNOWBUG_PIPE_%s_%u", prefix, (unsigned)pid);
	return to_os(std::u8string_view{(char8_t const*)buf, (size_t)len});
}

// 名前付きパイプを作る (`CreateNamedPipe` のラッパー。ハンドルは自動でクローズされる)
static auto create_named_pipe(OsStringView name) -> std::optional<PipeHandle> {
	auto security_attrs = SECURITY_ATTRIBUTES{ sizeof(SECURITY_ATTRIBUTES) };
	security_attrs.bInheritHandle = TRUE;

	auto h_pipe = CreateNamedPipe(name.data(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE, 1, PIPE_BUFFER_SIZE, PIPE_BUFFER_SIZE, 0, &security_attrs);
	if (h_pipe == INVALID_HANDLE_VALUE) {
		OutputDebugString(_T("CreateNamedPipe failed"));
		return std::nullopt;
	}
	assert(h_pipe != NULL);
	return PipeHandle{ h_pipe };
}

// -----------------------------------------------
// クライアントプロセス
// -----------------------------------------------

class KnowbugClientProcess {
public:
	PipeHandle input_pipe_;
	PipeHandle output_pipe_;
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

static auto start_client_process() -> std::optional<KnowbugClientProcess> {
	// クライアントプロセスの起動コマンドラインを構成する。
	auto hsp_dir = get_hsp_dir();
	auto name = hsp_dir + TEXT("knowbug_client.exe");

	auto cmdline = OsString{ TEXT("\"") };
	cmdline += name;
	cmdline += TEXT("\"");

	// クライアントとの通信用のパイプを作成する。
	// (input: サーバーからクライアントへ、output: クライアントからサーバーへ)
	auto input_pipe_name = compute_pipe_name("input");
	auto output_pipe_name = compute_pipe_name("output");
	auto input_pipe_opt = create_named_pipe(input_pipe_name);
	auto output_pipe_opt = create_named_pipe(output_pipe_name);
	if (!input_pipe_opt || !output_pipe_opt) {
		return std::nullopt;
	}

	cmdline += TEXT(" --input=");
	cmdline += input_pipe_name;
	cmdline += TEXT(" --output=");
	cmdline += output_pipe_name;

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
		std::move(input_pipe_opt).value(),
		std::move(output_pipe_opt).value(),
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
	bool input_connected_;
	bool output_connected_;

	std::optional<UINT_PTR> timer_opt_;

	HspObjectListEntity object_list_entity_;

public:
	KnowbugServerImpl(HSP3DEBUG* debug, HspObjects& objects, HINSTANCE instance, KnowbugStepController& step_controller)
		: debug_(debug)
		, objects_(objects)
		, instance_(instance)
		, step_controller_(step_controller)
		, started_(false)
, hidden_window_opt_()
		, client_process_opt_()
		, input_connected_(false)
		, output_connected_(false)
		, object_list_entity_()
	{
	}

	void start() override {
		if (std::exchange(started_, true)) {
			assert(false && u8"double start");
			return;
		}

		hidden_window_opt_ = create_hidden_window(instance_);

		client_process_opt_ = start_client_process();
		if (!client_process_opt_) {
			MessageBox(hidden_window_opt_->get(), TEXT("デバッグウィンドウの初期化に失敗しました。(クライアントプロセスを起動できません。)"), TEXT("knowbug"), MB_ICONERROR);
			return;
		}

		timer_opt_ = SetTimer(hidden_window_opt_->get(), 1, 16, NULL);
	}

	void will_exit() override {
if (hidden_window_opt_ && timer_opt_) {
			if (!KillTimer(hidden_window_opt_->get(), *timer_opt_)) {
				assert(false && "KillTimer");
			}
		}

		send_terminated_event();
	}

	void logmes(HspStringView text) override {
		auto utf8_text = to_utf8(text);
		send_output_event(utf8_text);
	}

	void debuggee_did_stop() override {
		send_stopped_event();
	}

	void read_client_output() {
		if (!client_process_opt_) {
			return;
		}

		auto h_pipe = client_process_opt_->output_pipe_.get();

		if (!output_connected_) {
			// クライアントプロセスがパイプに接続するのを待つ。
			// (`ConnectNamedPipe` は相手がパイプを開くのを待機する。
			//  すでにパイプが開かれている場合、特定のエラーコードで「失敗」するが、接続は確立している。)
			OutputDebugString(_T("Waiting for output connected\n"));
			if (!ConnectNamedPipe(h_pipe, NULL)) {
				auto err = GetLastError();
				if (err != ERROR_PIPE_CONNECTED) {
					client_process_opt_.reset();
					assert(false && "ConnectNamedPipe");
					return;
				}
			}
			output_connected_ = true;
		}

		static auto s_buffer = std::u8string{};
		if (s_buffer.size() == 0) {
			s_buffer.resize(1024 * 1024);
		}

		auto total_size = DWORD{};
		// (pipe, buf, bufSize, bytesRead, total, left)
		if (!PeekNamedPipe(h_pipe, nullptr, 0, nullptr, &total_size, nullptr)) {
			auto err = GetLastError();
			client_process_opt_.reset();
			if (err != ERROR_BROKEN_PIPE) {
				assert(false && "PeekNamedPipe");
			}
			return;
		}

		if (total_size == 0) {
			return;
		}

		auto read_size = DWORD{};
		if (!ReadFile(h_pipe, s_buffer.data(), (DWORD)s_buffer.size(), &read_size, LPOVERLAPPED{})) {
			auto err = GetLastError();
			client_process_opt_.reset();
			if (err != ERROR_BROKEN_PIPE) {
				assert(false && "ReadFile");
			}
			return;
		}

		static auto s_data = std::u8string{};

		auto chunk = std::u8string_view{ s_buffer.data(), (std::size_t)read_size };

		s_data += chunk;

		while (true) {
			auto message_opt = knowbug_protocol_parse(s_data);
			if (!message_opt) {
				break;
			}

			client_did_send_something(*message_opt);
		}
	}

	void client_did_send_something(KnowbugMessage const& message) {
		auto method = message.method();
		auto method_str = as_native(method);

		if (method == u8"initialize_notification") {
			client_did_initialize();
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

	void client_did_initialize() {
		send_initialized_event();
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

		object_list_entity_.toggle_expand((std::size_t)object_id);

		send_list_updated_events();
	}

	void client_did_list_details(int object_id) {
		if (object_id < 0) {
			assert(false && u8"bad object_id");
			return;
		}

		send_list_details_event((std::size_t)object_id);
	}

private:
	auto objects() -> HspObjects& {
		return objects_;
	}

	void send_message(KnowbugMessage const& message) {
		auto text = knowbug_protocol_serialize(message);

		if (text.size() >= MEMORY_BUFFER_SIZE) {
			// FIXME: ログ出力
			assert(false && u8"too large to send");
			return;
		}

		if (!client_process_opt_) {
			return;
		}

		// クライアントの標準入力に流す。
		auto h_pipe = client_process_opt_->input_pipe_.get();

		if (!input_connected_) {
			// (クライアントによる接続を待つ。output_pipe_と同様)
			OutputDebugString(_T("Waiting for input connected\n"));
			if (!ConnectNamedPipe(h_pipe, NULL)) {
				auto err = GetLastError();
				if (err != ERROR_PIPE_CONNECTED) {
					client_process_opt_.reset();
					assert(false && "ConnectNamedPipe");
					return;
				}
			}
			input_connected_ = true;
		}

		auto written_size = DWORD{};
		if (!WriteFile(h_pipe, text.data(), (DWORD)text.size(), &written_size, LPOVERLAPPED{})) {
			auto err = GetLastError();
			if (err != ERROR_BROKEN_PIPE && err != ERROR_NO_DATA) {
				assert(false && u8"WriteFile");
			}
			input_connected_ = false;
			return;
		}
		assert(written_size == text.size());
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
		auto diff = object_list_entity_.update(objects());

		for (auto i = std::size_t{}; i < diff.size(); i++) {
			auto const& delta = diff[i];

			auto message = KnowbugMessage::new_with_method(std::u8string{ u8"list_updated_event" });

			message.insert(
				std::u8string{ u8"kind" },
				std::u8string{ HspObjectListDelta::kind_to_string(delta.kind()) }
			);

			message.insert_int(
				std::u8string{ u8"object_id" },
				(int)delta.object_id()
			);

			message.insert_int(
				std::u8string{ u8"index" },
				(int)delta.index()
			);

			message.insert(
				std::u8string{ u8"name" },
				delta.name()
			);

			message.insert(
				std::u8string{ u8"value" },
				std::u8string{ delta.value() }
			);

			if (delta.kind() == HspObjectListDelta::Kind::Remove && delta.count() >= 2) {
				message.insert_int(
					std::u8string{ u8"count" },
					(int)delta.count()
				);
			}

			send_message(message);
		}
	}

	void send_list_details_event(std::size_t object_id) {
		auto text_opt = std::optional<std::u8string>{};

		auto path_opt = object_list_entity_.object_id_to_path(object_id);
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

	case WM_TIMER: {
		if (auto server = s_server.lock()) {
			server->read_client_output();
		}
		break;
	}
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}
