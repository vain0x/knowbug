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

class KnowbugView {
	Resource const& r_;

public:
	KnowbugView(Resource const& r)
		: r_(r)
	{
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

	void resize_main_window(int client_x, int client_y, bool repaint) {
		::resize_main_window(client_x, client_y, repaint, var_tree_view(), source_edit(), step_buttons());
	}

	void initialize_view_window_layout() {
		auto rc = window_to_client_rect(view_window());
		MoveWindow(view_edit(), 0, 0, rc.right, rc.bottom, !REPAINT);
	}

	void show_windows() {
		for (auto hwnd : windows()) {
			UpdateWindow(hwnd);
			ShowWindow(hwnd, SW_SHOW);
		}
	}

private:
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

	auto windows() const -> std::vector<HWND> {
		auto v = std::vector<HWND>{};
		v.emplace_back(main_window());
		v.emplace_back(view_window());
		return v;
	}
};

static auto get_knowbug_view() -> std::optional<KnowbugView> {
	if (!g_res) {
		return std::nullopt;
	}

	return std::make_optional(KnowbugView{ *g_res });
}

static auto windowHandles() -> std::vector<HWND>
{
	return std::vector<HWND> { g_res->mainWindow.get(), g_res->viewWindow.get() };
}

static void setEditStyle(HWND hEdit);

namespace View {

void update()
{
	auto view_box = ViewBoxImpl{ hViewEdit };

	g_res->tv->update_view_window(view_box);
}

} // namespace View

// ツリーノードのコンテキストメニュー
void VarTree_PopupMenu(HTREEITEM hItem, POINT pt)
{
	auto&& path_opt = g_res->tv->item_to_path(hItem);
	if (!path_opt) {
		return;
	}

	HMENU pop_menu;
	switch ((**path_opt).kind()) {
	case HspObjectKind::CallFrame:
		pop_menu = g_res->invokeMenu.get();
		break;
	case HspObjectKind::Log:
		pop_menu = g_res->logMenu.get();
		break;
	default:
		pop_menu = g_res->nodeMenu.get();
		break;
	}

	// ポップアップメニューを表示する
	auto const idSelected =
		TrackPopupMenuEx
			( pop_menu, (TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD)
			, pt.x, pt.y, g_res->mainWindow.get(), nullptr);

	switch ( idSelected ) {
		case 0:
			break;

		case IDC_NODE_UPDATE:
			View::update();
			break;

		case IDC_NODE_LOG: {
			Knowbug::add_object_text_to_log(*std::move(path_opt));
			break;
		}
		case IDC_LOG_SAVE:
			Knowbug::save_log();
			break;

		case IDC_LOG_CLEAR:
			Knowbug::clear_log();
			break;

		default:
			assert(false && u8"Unknown popup menu command ID");
			throw std::exception{};
	}
}

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

				case IDC_TOPMOST: {
					Menu_ToggleCheck(g_res->dialogMenu.get(), IDC_TOPMOST, g_config->bTopMost);
					for ( auto&& hwnd : windowHandles() ) {
						Window_SetTopMost(hwnd, g_config->bTopMost);
					}
					break;
				}
				case IDC_OPEN_CURRENT_SCRIPT:
					Knowbug::open_current_script_file();
					break;
				case IDC_OPEN_INI: {
					Knowbug::open_config_file();
					break;
				}
				case IDC_UPDATE: {
					View::update();
					break;
				}
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
			if ( (HWND)wp == hVarTree ) {
				auto pt = POINT { LOWORD(lp), HIWORD(lp) };
				if ( auto hItem = TreeView_GetItemAtPoint(hVarTree, pt) ) {
					VarTree_PopupMenu(hItem, pt);
					return TRUE;
				}
			}
			break;
		}
		case WM_NOTIFY: {
			auto const nmhdr = reinterpret_cast<LPNMHDR>(lp);
			if ( nmhdr->hwndFrom == hVarTree ) {
				switch ( nmhdr->code ) {
					// FIXME: フォーカスした要素の周囲を更新する
					case NM_DBLCLK:
					case NM_RETURN:
					case TVN_SELCHANGED:
						View::update();
						break;
				}
			}
			break;
		}
		case WM_SIZE:
			if (auto&& view_opt = get_knowbug_view()) {
				view_opt->resize_main_window(LOWORD(lp), HIWORD(lp), REPAINT);
			}
			break;
		case WM_ACTIVATE:
			{
				auto is_activated = LOWORD(wp) != 0;
				if (g_res && is_activated) {
					SetWindowPos(g_res->viewWindow.get(), hDlg, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				}
				break;
			}
		case WM_CREATE: return TRUE;
		case WM_CLOSE: return FALSE;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}
	return DefWindowProc(hDlg, msg, wp, lp);
}

LRESULT CALLBACK ViewDialogProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch ( msg ) {
		case WM_CREATE: return TRUE;
		case WM_CLOSE: return FALSE;
		case WM_SIZE:
			MoveWindow(hViewEdit, 0, 0, LOWORD(lp), HIWORD(lp), TRUE);
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

	if ( g_config->bTopMost ) {
		CheckMenuItem(g_res->dialogMenu.get(), IDC_TOPMOST, MF_CHECKED);
		for ( auto&& hwnd : windowHandles() ) {
			Window_SetTopMost(hwnd, true);
		}
	}

	if (auto&& view_opt = get_knowbug_view()) {
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
	View::update();
}

void update_source_view(OsStringView const& content) {
	Edit_SetText(hSrcLine, content.data());
}

void did_log_change() {
	if (g_res && g_res->tv && g_res->tv->log_is_selected()) {
		Dialog::View::update();
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
