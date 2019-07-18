//! knowbug の UI 関連

#pragma comment(lib, "comctl32.lib")

#include "resource.h"
#include "hpiutil/hspsdk/hspwnd.h"
#include "module/GuiUtility.h"
#include "module/handle_deleter.hpp"
#include "knowbug_app.h"
#include "knowbug_view.h"
#include "knowbug_view_tree.h"
#include "knowbug_config.h"
#include "HspObjectPath.h"
#include "platform.h"
#include "StepController.h"

#ifdef _M_X64
# define KNOWBUG_CPU_SUFFIX TEXT("(x64)")
#else //defined(_M_X64)
# define KNOWBUG_CPU_SUFFIX TEXT("(x86)")
#endif //defined(_M_X64)
#define KNOWBUG_TITLE TEXT("Knowbug")
#define KNOWBUG_VERSION TEXT("1.22.2 ") KNOWBUG_CPU_SUFFIX

static auto const KNOWBUG_MAIN_WINDOW_TITLE = KNOWBUG_TITLE TEXT(" ") KNOWBUG_VERSION;

static auto const KNOWBUG_VIEW_WINDOW_TITLE = TEXT("Knowbug View");

static auto const REPAINT = true;

static auto const STEP_BUTTON_COUNT = std::size_t{ 5 };

using StepButtonHandleArray = std::array<HWND, STEP_BUTTON_COUNT>;

using WindowHandle = window_handle_t;

using MenuHandle = menu_handle_t;

using FontHandle = gdi_obj_t;

static auto window_to_client_rect(HWND hwnd) -> RECT {
	RECT rc;
	GetClientRect(hwnd, &rc);
	return rc;
}

static auto create_main_font(KnowbugConfig const& config) -> gdi_obj_t {
	return gdi_obj_t{ Font_Create(as_view(config.fontFamily), config.fontSize, config.fontAntialias) };
}

static void resize_main_window(int client_x, int client_y, bool repaints, HWND tree_view, HWND source_edit, StepButtonHandleArray const& step_button_handles) {
	auto const source_edit_size_y = 50;
	auto const step_button_size_x = client_x / STEP_BUTTON_COUNT;
	auto const step_button_size_y = 20;
	auto const tree_view_size_y = client_y - (source_edit_size_y + step_button_size_y);

	MoveWindow(
		tree_view,
		0, 0,
		client_x, tree_view_size_y,
		repaints
	);

	MoveWindow(
		source_edit,
		0, tree_view_size_y,
		client_x, source_edit_size_y,
		repaints
	);

	for (auto i = std::size_t{}; i < step_button_handles.size(); ++i) {
		MoveWindow(
			step_button_handles[i],
			i * step_button_size_x, tree_view_size_y + source_edit_size_y,
			step_button_size_x, step_button_size_y,
			repaints
		);
	}
}

static auto select_save_log_file(HWND window) -> std::optional<OsString> {
	static auto const FILTER = TEXT("log text(*.txt;*.log)\0*.txt;*.log\0All files(*.*)\0*.*\0\0");

	auto path = Dialog_SaveFileName(
		window,
		FILTER,
		TEXT("log"),
		TEXT("hspdbg.log")
	);
	if (!path) {
		return std::nullopt;
	}

	return std::make_optional(*std::move(path));
}

class ViewEditControlImpl
	: public AbstractViewBox
{
	HWND view_edit_;

public:
	ViewEditControlImpl(HWND view_edit)
		: view_edit_(view_edit)
	{
	}

	auto current_scroll_line() const -> std::size_t override {
		return Edit_GetFirstVisibleLine(view_edit_);
	}

	bool at_bottom() const override {
		auto line_count = Edit_GetLineCount(view_edit_);

		// ウィンドウに30行ぐらい表示されていると仮定して、スクロールが一番下にありそうかどうか判定する。
		return line_count <= (int)current_scroll_line() + 30;
	}

	void scroll_to_line(std::size_t line_index) override {
		Edit_Scroll(view_edit_, (int)line_index, 0);
	}

	void scroll_to_bottom() override {
		auto line_count = Edit_GetLineCount(view_edit_);
		scroll_to_line(line_count);
	}

	void select_line(std::size_t line_index) override {
		auto start = Edit_LineIndex(view_edit_, (int)line_index);
		auto end = Edit_LineIndex(view_edit_, (int)line_index + 1);
		Edit_SetSel(view_edit_, start, end);
	}

	void set_text(OsStringView const& text) override {
		SetWindowText(view_edit_, text.data());
	}
};

class KnowbugViewImpl
	: public KnowbugView
{
	WindowHandle main_window_, view_window_;
	MenuHandle main_menu_, node_menu_, call_frame_menu_, log_menu_;
	HWND tree_view_;
	HWND source_edit_;
	HWND view_edit_;
	std::unique_ptr<VarTreeViewControl> var_tree_view_control_;

	StepButtonHandleArray step_buttons_;
	FontHandle main_font_;

	KnowbugConfig const& config_;
	ViewEditControlImpl view_edit_control_;
	bool top_most_;

public:
	KnowbugViewImpl(
		WindowHandle&& main_window,
		WindowHandle&& view_window,
		MenuHandle&& main_menu,
		MenuHandle&& node_menu,
		MenuHandle&& call_frame_menu,
		MenuHandle&& log_menu,
		HWND var_tree,
		HWND source_edit,
		HWND view_edit,
		std::unique_ptr<VarTreeViewControl>&& var_tree_view_control,
		StepButtonHandleArray const& step_buttons,
		FontHandle&& main_font,
		KnowbugConfig const& config
	)
		: main_window_(std::move(main_window))
		, view_window_(std::move(view_window))
		, main_menu_(std::move(main_menu))
		, node_menu_(std::move(node_menu))
		, call_frame_menu_(std::move(call_frame_menu))
		, log_menu_(std::move(log_menu))
		, tree_view_(var_tree)
		, source_edit_(source_edit)
		, view_edit_(view_edit)
		, var_tree_view_control_(std::move(var_tree_view_control))
		, step_buttons_(step_buttons)
		, main_font_(std::move(main_font))
		, config_(config)
		, view_edit_control_(view_edit_)
		, top_most_(false)
	{
	}

private:
	// 初期化:

	void set_windows_top_most() {
		if (config().bTopMost) {
			toggle_windows_top_most();
		}
	}

	void apply_main_font() {
		for (auto hwnd : {
			source_edit(),
			view_edit()
		}) {
			SendMessage(hwnd, WM_SETFONT, (WPARAM)main_font(), !REPAINT);
		}
	}

	void initialize_main_window_layout() {
		auto rc = window_to_client_rect(main_window());
		resize_main_window(rc.right, rc.bottom, !REPAINT);
	}

	void initialize_view_window_layout() {
		auto rc = window_to_client_rect(view_window());
		resize_view_window(rc.right, rc.bottom, !REPAINT);
	}

	void show_windows() {
		for (auto hwnd : windows()) {
			UpdateWindow(hwnd);
			ShowWindow(hwnd, SW_SHOW);
		}
	}

public:
	// 更新:

	void initialize() override {
		set_windows_top_most();
		apply_main_font();
		initialize_main_window_layout();
		initialize_view_window_layout();
		show_windows();
	}

	void update() override {
		update_view_edit();
	}

	void update_source_edit(OsStringView const& content) override {
		Edit_SetText(source_edit(), content.data());
	}

	void did_log_change() override {
		if (var_tree_view_control().log_is_selected()) {
			update_view_edit();
		}
	}

	// UI 操作:

	auto select_save_log_file() -> std::optional<OsString> override {
		return (::select_save_log_file)(main_window());
	}

	void notify_save_failure() override {
		static auto const MESSAGE = TEXT("ログの保存に失敗しました。");
		MessageBox(main_window(), MESSAGE, KNOWBUG_TITLE, MB_OK);
	}

	// UI イベント:

	void resize_main_window(int client_x, int client_y, bool repaint) {
		::resize_main_window(client_x, client_y, repaint, var_tree_view(), source_edit(), step_buttons());
	}

	void resize_view_window(int client_x, int client_y, bool repaint) {
		MoveWindow(view_edit(), 0, 0, client_x, client_y, repaint);
	}

	void toggle_windows_top_most() {
		auto top_most = !top_most_;
		top_most_ = top_most;

		CheckMenuItem(main_menu(), IDC_TOPMOST, top_most ? MF_CHECKED : MF_UNCHECKED);
		for (auto hwnd : windows()) {
			Window_SetTopMost(hwnd, top_most);
		}
	}

	void update_view_edit() {
		var_tree_view_control().update_view_window(view_edit_control());
	}

	auto open_context_menu(HWND hwnd, POINT point, KnowbugApp& app) -> bool {
		if (hwnd == var_tree_view()) {
			return open_context_menu_var_tree_view(point, app);
		}
		return false;
	}

	auto open_context_menu_var_tree_view(POINT const& point, KnowbugApp& app) -> bool {
		auto&& path_opt = point_to_path(point);
		if (!path_opt) {
			return false;
		}

		auto popup_menu = get_node_menu(**path_opt);
		auto selected_id = open_popup_menu(popup_menu, point);
		if (selected_id == FALSE) {
			return false;
		}

		execute_popup_menu_action(selected_id, **path_opt, app);
		return true;
	}

	auto process_main_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, KnowbugApp& app) -> LRESULT override {
		switch (msg) {
		case WM_USER:
			update_view_edit();
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wp)) {
			case IDC_BTN1:
				app.step_run(StepControl::run());
				break;

			case IDC_BTN2:
				app.step_run(StepControl::step_in());
				break;

			case IDC_BTN3:
				app.step_run(StepControl::stop());
				break;

			case IDC_BTN4:
				app.step_run(StepControl::step_over());
				break;

			case IDC_BTN5:
				app.step_run(StepControl::step_out());
				break;

			case IDC_TOPMOST:
				toggle_windows_top_most();
				break;

			case IDC_OPEN_CURRENT_SCRIPT:
				app.open_current_script_file();
				break;

			case IDC_OPEN_INI:
				app.open_config_file();
				break;

			case IDC_UPDATE:
				update_view_edit();
				break;

			case IDC_OPEN_KNOWBUG_REPOS:
				app.open_knowbug_repository();
				break;

			case IDC_GOTO_LOG:
				// FIXME: ログノードを選択する。
				break;

			case IDC_GOTO_SCRIPT:
				// FIXME: スクリプトノードを選択する。
				break;
			}
			break;

		case WM_CONTEXTMENU:
			{
				auto done = open_context_menu((HWND)wp, POINT{ LOWORD(lp), HIWORD(lp) }, app);
				if (done) {
					return TRUE;
				}
			}
			break;

		case WM_NOTIFY:
			{
				auto nmhdr = (LPNMHDR)lp;
				if (nmhdr->hwndFrom == var_tree_view()) {
					switch (nmhdr->code) {
					case NM_CLICK:
					case NM_RETURN:
					case TVN_SELCHANGED:
						// 現時点ではまだツリービューがメッセージを処理していないので、
						// クリックされた要素に選択が移動していない。
						// ここで更新用のメッセージをキューに post することにより、
						// メッセージがツリービューに処理された後に更新を起こす。
						PostMessage(hwnd, WM_USER, 0, 0);
						break;
					}
				}
			}
			break;

		case WM_SIZE:
			resize_main_window(LOWORD(lp), HIWORD(lp), REPAINT);
			break;

		case WM_ACTIVATE:
			{
				auto is_activated = LOWORD(wp) != 0;
				if (is_activated) {
					move_view_window_to_front();
				}
			}
			break;

		case WM_CREATE:
			return TRUE;

		case WM_CLOSE:
			return FALSE;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		}
		return DefWindowProc(hwnd, msg, wp, lp);
	}

	auto process_view_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, KnowbugApp& app) -> LRESULT override {
		switch (msg) {
		case WM_CREATE:
			return TRUE;

		case WM_CLOSE:
			return FALSE;

		case WM_SIZE:
			resize_view_window(LOWORD(lp), HIWORD(lp), REPAINT);
			break;
		}
		return DefWindowProc(hwnd, msg, wp, lp);
	}

private:
	auto config() const -> KnowbugConfig const& {
		return config_;
	}

	auto main_font() const -> HGDIOBJ {
		return main_font_.get();
	}

	auto main_window() const -> HWND {
		return main_window_.get();
	}

	auto view_window() const -> HWND {
		return view_window_.get();
	}

	auto main_menu() const -> HMENU {
		return main_menu_.get();
	}

	auto var_tree_view() const -> HWND {
		return tree_view_;
	}

	auto source_edit() const -> HWND {
		return source_edit_;
	}

	auto step_buttons() const -> StepButtonHandleArray const& {
		return step_buttons_;
	}

	auto view_edit() const -> HWND {
		return view_edit_;
	}

	auto var_tree_view_control() -> VarTreeViewControl& {
		return *var_tree_view_control_;
	}

	auto view_edit_control() -> AbstractViewBox& {
		return view_edit_control_;
	}

	auto windows() const -> std::vector<HWND> {
		auto v = std::vector<HWND>{};
		v.emplace_back(main_window());
		v.emplace_back(view_window());
		return v;
	}

	void move_view_window_to_front() {
		SetWindowPos(view_window(), main_window(), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	auto point_to_path(POINT const& point) -> std::optional<std::shared_ptr<HspObjectPath const>> {
		auto tree_item = TreeView_GetItemAtPoint(var_tree_view(), point);
		if (!tree_item) {
			return std::nullopt;
		}

		return var_tree_view_control().item_to_path(tree_item);
	}

	auto get_node_menu(HspObjectPath const& path) -> HMENU {
		switch (path.kind()) {
		case HspObjectKind::CallFrame:
			return call_frame_menu_.get();

		case HspObjectKind::Log:
			return log_menu_.get();

		default:
			return node_menu_.get();
		}
	}

	auto open_popup_menu(HMENU popup_menu, POINT const& point) -> int {
		return TrackPopupMenuEx(
			popup_menu,
			TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
			point.x, point.y,
			main_window(),
			nullptr
		);
	}

	void execute_popup_menu_action(int selected_id, HspObjectPath const& path, KnowbugApp& app) {
		switch (selected_id) {
		case IDC_NODE_UPDATE:
			update_view_edit();
			return;

		case IDC_NODE_LOG:
			app.add_object_text_to_log(path);
			return;

		case IDC_LOG_SAVE:
			app.save_log();
			return;

		case IDC_LOG_CLEAR:
			app.clear_log();
			return;

		default:
			assert(false && u8"Unknown popup menu command ID");
			throw std::exception{};
		}
	}
};

// メインウィンドウのコールバック関数
LRESULT CALLBACK process_main_window(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp) {
	if (auto&& app = KnowbugApp::instance()) {
		return app->view().process_main_window(hDlg, msg, wp, lp, *app);
	}
	return DefWindowProc(hDlg, msg, wp, lp);
}

LRESULT CALLBACK process_view_window(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp) {
	if (auto&& app = KnowbugApp::instance()) {
		return app->view().process_view_window(hDlg, msg, wp, lp, *app);
	}
	return DefWindowProc(hDlg, msg, wp, lp);
}

auto KnowbugView::create(KnowbugConfig const& config, HINSTANCE instance, HspObjects& objects, HspObjectTree& object_tree) -> std::unique_ptr<KnowbugView> {
	auto const display_x = GetSystemMetrics(SM_CXSCREEN);
	auto const display_y = GetSystemMetrics(SM_CYSCREEN);

	auto const main_size_x = 234;
	auto const main_size_y = 380;
	auto const main_pos_x = display_x - main_size_x;
	auto const main_pos_y = 0;
	auto const view_size_x = config.viewSizeX;
	auto const view_size_y = config.viewSizeY;
	auto const view_pos_x = !config.viewPosXIsDefault ? config.viewPosX : display_x - main_size_x - view_size_x;
	auto const view_pos_y = !config.viewPosYIsDefault ? config.viewPosY : 0;

	auto const tab_width = config.tabwidth;
	auto main_font = create_main_font(config);

	// ビューウィンドウ
	auto view_window = window_handle_t{
		Window_Create(
			OsStringView{ TEXT("KnowbugViewWindow") },
			::process_view_window,
			OsStringView{ KNOWBUG_VIEW_WINDOW_TITLE },
			WS_THICKFRAME,
			view_size_x, view_size_y,
			view_pos_x, view_pos_y,
			instance
		) };
	SetWindowLongPtr(view_window.get(), GWL_EXSTYLE, GetWindowLongPtr(view_window.get(), GWL_EXSTYLE) | WS_EX_TOOLWINDOW);

	auto const view_pane = CreateDialog(
		instance,
		(LPCTSTR)IDD_VIEW_PANE,
		view_window.get(),
		(DLGPROC)::process_view_window
	);

	auto const view_edit = GetDlgItem(view_pane, IDC_VIEW);
	Edit_SetTabLength(view_edit, tab_width);

	ShowWindow(view_pane, SW_SHOW);

	// メインウィンドウ
	auto main_window = window_handle_t{
		Window_Create(
			OsStringView{ TEXT("KnowbugMainWindow") },
			::process_main_window,
			OsStringView{ KNOWBUG_MAIN_WINDOW_TITLE },
			WS_THICKFRAME,
			main_size_x, main_size_y,
			main_pos_x, main_pos_y,
			instance
		) };

	auto const main_pane = CreateDialog(
		instance,
		(LPCTSTR)IDD_MAIN_PANE,
		main_window.get(),
		(DLGPROC)::process_main_window
	);
	ShowWindow(main_pane, SW_SHOW);

	// メニューバー
	auto main_menu = menu_handle_t{ LoadMenu(instance, (LPCTSTR)IDR_MAIN_MENU) };
	SetMenu(main_window.get(), main_menu.get());

	// ポップメニュー
	auto const node_menu_Bar = LoadMenu(instance, (LPCTSTR)IDR_NODE_MENU);

	auto node_menu = menu_handle_t{ GetSubMenu(node_menu_Bar, 0) };
	auto call_frame_menu = menu_handle_t{ GetSubMenu(node_menu_Bar, 1) };
	auto log_menu = menu_handle_t{ GetSubMenu(node_menu_Bar, 2) };

	// ツリービュー
	auto const tree_view = GetDlgItem(main_pane, IDC_VARTREE);

	auto var_tree_view_control = VarTreeViewControl::create(objects, object_tree, tree_view);

	auto const source_edit = GetDlgItem(main_pane, IDC_SRC_LINE);

	// ステップボタン
	auto const step_buttons = StepButtonHandleArray{
		GetDlgItem(main_pane, IDC_BTN1),
		GetDlgItem(main_pane, IDC_BTN2),
		GetDlgItem(main_pane, IDC_BTN3),
		GetDlgItem(main_pane, IDC_BTN4),
		GetDlgItem(main_pane, IDC_BTN5)
	};

	auto view = std::make_unique<KnowbugViewImpl>(
		std::move(main_window),
		std::move(view_window),
		std::move(main_menu),
		std::move(node_menu),
		std::move(call_frame_menu),
		std::move(log_menu),
		tree_view,
		source_edit,
		view_edit,
		std::move(var_tree_view_control),
		step_buttons,
		std::move(main_font),
		config
	);

	return std::unique_ptr<KnowbugView>{ std::move(view) };
}
