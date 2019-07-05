//! knowbug の UI 関連

#pragma comment(lib, "comctl32.lib")

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <fstream>

#include "main.h"
#include "resource.h"
#include "hpiutil/hspsdk/hspwnd.h"
#include "module/GuiUtility.h"
#include "module/strf.h"
#include "WrapCall/WrapCall.h"

#include "dialog.h"
#include "vartree.h"
#include "config_mng.h"
#include "DebugInfo.h"
#include "SourceFileResolver.h"
#include "HspObjectPath.h"

#include "module/supio/supio.h"

#ifdef _M_X64
# define KnowbugPlatformString TEXT("(x64)")
#else //defined(_M_X64)
# define KnowbugPlatformString TEXT("(x86)")
#endif //defined(_M_X64)
#define KnowbugAppName TEXT("Knowbug")
#define KnowbugVersion TEXT("1.22.2 ") KnowbugPlatformString
static auto KnowbugMainWindowTitle = KnowbugAppName TEXT(" ") KnowbugVersion;
static auto KnowbugViewWindowTitle = TEXT("Knowbug View");

static auto const REPAINT = true;

static auto const STEP_BUTTON_COUNT = std::size_t{ 5 };

using StepButtonHandleArray = std::array<HWND, STEP_BUTTON_COUNT>;

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

class ViewBoxImpl
	: public AbstractViewBox
{
	HWND view_edit_;

public:
	ViewBoxImpl(HWND view_edit)
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

namespace Dialog
{

static HWND hVarTree;
static HWND hSrcLine;
static HWND hViewEdit;

struct Resource
{
	window_handle_t mainWindow, viewWindow;
	menu_handle_t dialogMenu, nodeMenu, invokeMenu, logMenu;
	std::unique_ptr<VarTreeViewControl> tv;

	StepButtonHandleArray stepButtons;
	gdi_obj_t font;
};

static auto g_res = unique_ptr<Resource> {};

static auto g_top_most = std::make_unique<bool>(false);

class KnowbugView {
	Resource const& r_;
	KnowbugConfig const& config_;
	ViewBoxImpl view_box_;

public:
	KnowbugView(Resource const& r, KnowbugConfig const& config)
		: r_(r)
		, config_(config)
		, view_box_(hViewEdit)
	{
	}

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

	// 更新:

	// UI イベント:

	void resize_main_window(int client_x, int client_y, bool repaint) {
		::resize_main_window(client_x, client_y, repaint, var_tree_view(), source_edit(), step_buttons());
	}

	void resize_view_window(int client_x, int client_y, bool repaint) {
		MoveWindow(view_edit(), 0, 0, client_x, client_y, repaint);
	}

	void toggle_windows_top_most() {
		auto top_most = !*g_top_most;
		*g_top_most = top_most;

		CheckMenuItem(dialog_menu(), IDC_TOPMOST, top_most ? MF_CHECKED : MF_UNCHECKED);
		for (auto hwnd : windows()) {
			Window_SetTopMost(hwnd, top_most);
		}
	}

	void update_view_edit() {
		var_tree_view_control().update_view_window(view_box());
	}

	void did_main_window_activate(bool is_activated) {
		if (!is_activated) {
			return;
		}

		move_view_window_to_front();
	}

	void did_notify(LPNMHDR nmhdr) {
		if (nmhdr->hwndFrom == var_tree_view()) {
			// FIXME: フォーカスした要素の周囲を更新する
			switch (nmhdr->code) {
			case NM_DBLCLK:
			case NM_RETURN:
			case TVN_SELCHANGED:
				update_view_edit();
				break;
			}
		}
	}

	auto open_context_menu(HWND hwnd, POINT point) -> bool {
		if (hwnd == var_tree_view()) {
			return open_context_menu_var_tree_view(point);
		}
		return false;
	}

	auto open_context_menu_var_tree_view(POINT const& point) -> bool {
		auto&& path_opt = point_to_path(point);
		if (!path_opt) {
			return false;
		}

		auto popup_menu = get_node_menu(**path_opt);
		auto selected_id = open_popup_menu(popup_menu, point);
		if (selected_id == FALSE) {
			return false;
		}

		execute_popup_menu_action(selected_id, **path_opt);
		return true;
	}

private:
	auto config() const -> KnowbugConfig const& {
		return config_;
	}

	auto main_font() const -> HGDIOBJ {
		return r_.font.get();
	}

	auto main_window() const -> HWND {
		return r_.mainWindow.get();
	}

	auto view_window() const -> HWND {
		return r_.viewWindow.get();
	}

	auto dialog_menu() const -> HMENU {
		return r_.dialogMenu.get();
	}

	auto var_tree_view() const -> HWND {
		return hVarTree;
	}

	auto source_edit() const -> HWND {
		return hSrcLine;
	}

	auto step_buttons() const -> StepButtonHandleArray const& {
		return r_.stepButtons;
	}

	auto view_edit() const -> HWND {
		return hViewEdit;
	}

	auto var_tree_view_control() -> VarTreeViewControl& {
		return *r_.tv;
	}

	auto view_box() -> AbstractViewBox& {
		return view_box_;
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
			return r_.invokeMenu.get();
			break;

		case HspObjectKind::Log:
			return r_.logMenu.get();

		default:
			return r_.nodeMenu.get();
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

	void execute_popup_menu_action(int selected_id, HspObjectPath const& path) {
		switch (selected_id) {
		case IDC_NODE_UPDATE:
			update_view_edit();
			return;

		case IDC_NODE_LOG:
			Knowbug::add_object_text_to_log(path);
			return;

		case IDC_LOG_SAVE:
			Knowbug::save_log();
			return;

		case IDC_LOG_CLEAR:
			Knowbug::clear_log();
			return;

		default:
			assert(false && u8"Unknown popup menu command ID");
			throw std::exception{};
		}
	}
};

static auto get_knowbug_view() -> std::optional<KnowbugView> {
	if (!g_res) {
		return std::nullopt;
	}

	return std::make_optional(KnowbugView{ *g_res, *g_config });
}

static auto windowHandles() -> std::vector<HWND>
{
	return std::vector<HWND> { g_res->mainWindow.get(), g_res->viewWindow.get() };
}

static void setEditStyle(HWND hEdit);

// メインウィンドウのコールバック関数
LRESULT CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch ( msg ) {
		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
				case IDC_BTN1: Knowbug::step_run(StepControl::run());       break;
				case IDC_BTN2: Knowbug::step_run(StepControl::step_in());   break;
				case IDC_BTN3: Knowbug::step_run(StepControl::stop());      break;
				case IDC_BTN4: Knowbug::step_run(StepControl::step_over()); break;
				case IDC_BTN5: Knowbug::step_run(StepControl::step_out());  break;

				case IDC_TOPMOST:
					if (auto&& view_opt = get_knowbug_view()) {
						view_opt->toggle_windows_top_most();
					}
					break;

				case IDC_OPEN_CURRENT_SCRIPT:
					Knowbug::open_current_script_file();
					break;
				case IDC_OPEN_INI: {
					Knowbug::open_config_file();
					break;
				}
				case IDC_UPDATE:
					if (auto&& view_opt = get_knowbug_view()) {
						view_opt->update_view_edit();
					}
					break;

				case IDC_OPEN_KNOWBUG_REPOS: {
					Knowbug::open_knowbug_repository();
					break;
				}
				case IDC_GOTO_LOG: {
					// FIXME: ログノードを選択する。
					break;
				}
				case IDC_GOTO_SCRIPT: {
					// FIXME: スクリプトノードを選択する。
					break;
				}
			}
			break;

		case WM_CONTEXTMENU: {
			if (auto&& view_opt = get_knowbug_view()) {
				auto done = view_opt->open_context_menu((HWND)wp, POINT{ LOWORD(lp), HIWORD(lp) });
				if (done) {
					return TRUE;
				}
			}
			break;
		}
		case WM_NOTIFY:
			if (auto&& view_opt = get_knowbug_view()) {
				view_opt->did_notify((LPNMHDR)lp);
			}
			break;

		case WM_SIZE:
			if (auto&& view_opt = get_knowbug_view()) {
				view_opt->resize_main_window(LOWORD(lp), HIWORD(lp), REPAINT);
			}
			break;

		case WM_ACTIVATE:
			if (auto&& view_opt = get_knowbug_view()) {
				auto is_activated = LOWORD(wp) != 0;
				view_opt->did_main_window_activate(is_activated);
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
	return DefWindowProc(hDlg, msg, wp, lp);
}

LRESULT CALLBACK ViewDialogProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch ( msg ) {
		case WM_CREATE:
			return TRUE;

		case WM_CLOSE:
			return FALSE;

		case WM_SIZE:
			if (auto&& view_opt = get_knowbug_view()) {
				view_opt->resize_view_window(LOWORD(lp), HIWORD(lp), REPAINT);
			}
			break;
	}
	return DefWindowProc(hDlg, msg, wp, lp);
}

void Dialog::createMain(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspObjectTree& object_tree)
{
	auto const dispx = GetSystemMetrics(SM_CXSCREEN);
	auto const dispy = GetSystemMetrics(SM_CYSCREEN);

	auto const mainSizeX = 234, mainSizeY = 380;
	auto const viewSizeX = g_config->viewSizeX, viewSizeY = g_config->viewSizeY;
	auto const viewPosX = !g_config->viewPosXIsDefault ? g_config->viewPosX : dispx - mainSizeX - viewSizeX;
	auto const viewPosY = !g_config->viewPosYIsDefault ? g_config->viewPosY : 0;

	//ビューウィンドウ
	auto hViewWnd = window_handle_t {
		Window_Create
			( OsStringView{ TEXT("KnowbugViewWindow") }, ViewDialogProc
			, OsStringView{ KnowbugViewWindowTitle }, (WS_THICKFRAME)
			, viewSizeX, viewSizeY
			, viewPosX, viewPosY
			, Knowbug::getInstance()
			) };
	SetWindowLongPtr(hViewWnd.get(), GWL_EXSTYLE
		, GetWindowLongPtr(hViewWnd.get(), GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
	{
		auto const hPane =
			CreateDialog(Knowbug::getInstance()
				, (LPCTSTR)IDD_VIEW_PANE
				, hViewWnd.get(), (DLGPROC)ViewDialogProc);
		hViewEdit = GetDlgItem(hPane, IDC_VIEW);
		setEditStyle(hViewEdit);

		ShowWindow(hPane, SW_SHOW);
	}

	//メインウィンドウ
	auto hDlgWnd = window_handle_t {
		Window_Create
			( OsStringView{ TEXT("KnowbugMainWindow") }, DlgProc
			, OsStringView{ KnowbugMainWindowTitle }, WS_THICKFRAME
			, mainSizeX, mainSizeY
			, dispx - mainSizeX, 0
			, Knowbug::getInstance()
			) };
	{
		auto const hPane =
			CreateDialog(Knowbug::getInstance()
				, (LPCTSTR)IDD_MAIN_PANE
				, hDlgWnd.get(), (DLGPROC)DlgProc);
		ShowWindow(hPane, SW_SHOW);

		//メニューバー
		auto hDlgMenu = menu_handle_t { LoadMenu(Knowbug::getInstance(), (LPCTSTR)IDR_MAIN_MENU) };
		SetMenu(hDlgWnd.get(), hDlgMenu.get());

		//ポップメニュー
		auto const hNodeMenuBar = LoadMenu(Knowbug::getInstance(), (LPCTSTR)IDR_NODE_MENU);

		//いろいろ
		hVarTree = GetDlgItem(hPane, IDC_VARTREE);
		hSrcLine = GetDlgItem(hPane, IDC_SRC_LINE);

		// メンバの順番に注意
		g_res.reset(new Resource
			{ std::move(hDlgWnd)
			, std::move(hViewWnd)
			, std::move(hDlgMenu)
			, menu_handle_t { GetSubMenu(hNodeMenuBar, 0) } // node
			, menu_handle_t { GetSubMenu(hNodeMenuBar, 1) } // invoke
			, menu_handle_t { GetSubMenu(hNodeMenuBar, 2) } // log
			, VarTreeViewControl::create(objects, object_tree, hVarTree)
			, {{
				  GetDlgItem(hPane, IDC_BTN1)
				, GetDlgItem(hPane, IDC_BTN2)
				, GetDlgItem(hPane, IDC_BTN3)
				, GetDlgItem(hPane, IDC_BTN4)
				, GetDlgItem(hPane, IDC_BTN5) }}
			, create_main_font(*g_config)
			});
	}

	if (auto&& view_opt = get_knowbug_view()) {
		view_opt->set_windows_top_most();
		view_opt->apply_main_font();
		view_opt->initialize_main_window_layout();
		view_opt->initialize_view_window_layout();
		view_opt->show_windows();
	}
}

void Dialog::destroyMain()
{
	g_res.reset();
}

// 一時停止時に dbgnotice から呼ばれる
void update()
{
	if (auto&& view_opt = get_knowbug_view()) {
		view_opt->update_view_edit();
	}
}

void update_source_view(OsStringView const& content) {
	Edit_SetText(hSrcLine, content.data());
}

void did_log_change() {
	if (g_res && g_res->tv && g_res->tv->log_is_selected()) {
		if (auto&& view_opt = get_knowbug_view()) {
			view_opt->update_view_edit();
		}
	}
}

void setEditStyle( HWND hEdit )
{
	Edit_SetTabLength(hEdit, g_config->tabwidth);
}

auto confirm_to_clear_log() -> bool {
	if (g_config->warnsBeforeClearingLog) {
		auto msg = TEXT("ログをすべて消去しますか？");
		auto ok = MessageBox(g_res->mainWindow.get(), msg, KnowbugAppName, MB_OKCANCEL) == IDOK;

		if (!ok) {
			return false;
		}
	}
	return true;
}

auto select_save_log_file() -> std::optional<OsString> {
	static auto const filter =
		TEXT("log text(*.txt;*.log)\0*.txt;*.log\0All files(*.*)\0*.*\0\0");

	auto path = Dialog_SaveFileName(
		g_res->mainWindow.get(),
		filter,
		TEXT("log"),
		TEXT("hspdbg.log")
	);

	if (!path) {
		return std::nullopt;
	}

	return std::make_optional(*std::move(path));
}

void notify_save_failure() {
	auto msg = TEXT("ログの保存に失敗しました。");
	MessageBox(g_res->mainWindow.get(), msg, KnowbugAppName, MB_OK);
}

} // namespace Dialog
