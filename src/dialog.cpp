
#pragma comment(lib, "comctl32.lib")

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <fstream>

#include "main.h"
#include "resource.h"
#include "hspwnd.h"
#include "module/supio/supio.h"
#include "module/GuiUtility.h"
#include "module/strf.h"
#include "WrapCall/WrapCall.h"

#include "dialog.h"
#include "vartree.h"
#include "config_mng.h"
#include "DebugInfo.h"

#ifdef _M_X64
# define KnowbugPlatformString "(x64)"
#else //defined(_M_X64)
# define KnowbugPlatformString "(x86)"
#endif //defined(_M_X64)
#define KnowbugAppName "Knowbug"
#define KnowbugVersion "v1.22" " " KnowbugPlatformString
static char const* const KnowbugMainWindowTitle = KnowbugAppName " " KnowbugVersion;
static char const* const KnowbugViewWindowTitle = "Knowbug View";
static char const* const KnowbugRepoUrl = "https://github.com/vain0/knowbug";

namespace Dialog
{

static size_t const countStepButtons = 5;

static HWND hVarTree;
static HWND hSrcLine;
static HWND hViewEdit;

struct Resource
{
	window_handle_t mainWindow, viewWindow;
	menu_handle_t dialogMenu, nodeMenu, invokeMenu, logMenu;
	unique_ptr<VTView> tv;

	std::array<HWND, countStepButtons> stepButtons;
	gdi_obj_t font;
};
static unique_ptr<Resource> g_res;

HWND getVarTreeHandle() { return hVarTree; }

static auto windowHandles() -> std::vector<HWND>
{
	return std::vector<HWND> { g_res->mainWindow.get(), g_res->viewWindow.get() };
}

static void setEditStyle(HWND hEdit, int maxlen);

namespace View {

void setText(char const* text)
{
	SetWindowText(hViewEdit, text);
};

void scroll(int y, int x)
{
	Edit_Scroll(hViewEdit, y, x);
}

void scrollBottom()
{
	scroll(Edit_GetLineCount(hViewEdit), 0);
}

void selectLine(size_t index)
{
	Edit_SetSel(hViewEdit
		, Edit_LineIndex(hViewEdit, index)
		, Edit_LineIndex(hViewEdit, index + 1));
}

void saveCurrentCaret()
{
	g_res->tv->saveCurrentViewCaret(Edit_GetFirstVisibleLine(hViewEdit));
}


void update()
{
	g_res->tv->updateViewWindow();
}

} // namespace View

bool logsCalling()
{
	return g_config->logsInvocation;
}

namespace LogBox {

	void clear()
	{
		if ( !g_config->warnsBeforeClearingLog
			|| MessageBox(g_res->mainWindow.get()
					, "ログをすべて消去しますか？", KnowbugAppName, MB_OKCANCEL
					) == IDOK
		) {
			VTRoot::log()->clear();
		}
	}

	void save(char const* filepath) {
		if ( !VTRoot::log()->save(filepath) ) {
			MessageBox(g_res->mainWindow.get()
				, "ログの保存に失敗しました。", KnowbugAppName, MB_OK);
		}
	}
	void save() {
		char const* const filter =
			"log text(*.txt;*.log)\0*.txt;*.log\0All files(*.*)\0*.*\0\0";
		if ( auto&& path = Dialog_SaveFileName(g_res->mainWindow.get()
				, filter, "log", "hspdbg.log" )
			) {
			save(path->c_str());
		}
	}
} //namespace LogBox

// ソース小窓の更新
static void UpdateCurInfEdit(char const* filepath, int iLine)
{
	if ( !filepath || iLine < 0 ) return;
	auto const&& curinf = DebugInfo::formatCurInfString(filepath, iLine);

	if ( auto&& p = VTRoot::script()->fetchScriptLine(filepath, iLine) ) {
		SetWindowText(hSrcLine, (curinf + "\r\n" + *p).c_str());

	} else {
		SetWindowText(hSrcLine, curinf.c_str());
	}
}

static void CurrentUpdate()
{
	UpdateCurInfEdit(g_dbginfo->curFileName(), g_dbginfo->curLine());
}

// ツリーノードのコンテキストメニュー
void VarTree_PopupMenu(HTREEITEM hItem, POINT pt)
{
	struct GetPopMenu
		: public VTNodeData::Visitor
	{
		void fInvoke(VTNodeInvoke const&) override
		{
			hPop = g_res->invokeMenu.get();
		}
		void fLog(VTNodeLog const&) override
		{
			hPop = g_res->logMenu.get();
		}
		HMENU apply(VTNodeData const& node)
		{
			hPop = g_res->nodeMenu.get(); // default
			node.acceptVisitor(*this);
			return hPop;
		}
	private:
		HMENU hPop;
	};

	auto&& node = g_res->tv->tryGetNodeData(hItem);
	if ( !node ) return;
	HMENU const hPop = GetPopMenu {}.apply(*node);

	// ポップアップメニューを表示する
	int const idSelected =
		TrackPopupMenuEx
			( hPop, (TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD)
			, pt.x, pt.y, g_res->mainWindow.get(), nullptr);

	switch ( idSelected ) {
		case 0: break;
		case IDC_NODE_UPDATE: View::update(); break;
		case IDC_NODE_LOG: {
			Knowbug::logmes(g_res->tv->getItemVarText(hItem)->c_str());
			break;
		}
#ifdef with_WrapCall
		case IDC_NODE_STEP_OUT: {
			if ( auto&& nodeInvoke = dynamic_cast<VTNodeInvoke const*>(node) ) {
				// 対象が呼び出された階層まで進む
				Knowbug::runStepReturn(nodeInvoke->callinfo().sublev);
			}
			break;
		}
#endif //defined(with_WrapCall)
		case IDC_LOG_AUTO_SCROLL: {
			Menu_ToggleCheck(hPop, IDC_LOG_AUTO_SCROLL, g_config->scrollsLogAutomatically);
			break;
		}
		case IDC_LOG_INVOCATION: {
			Menu_ToggleCheck(hPop, IDC_LOG_INVOCATION, g_config->logsInvocation);
			break;
		}
		case IDC_LOG_SAVE: LogBox::save(); break;
		case IDC_LOG_CLEAR: LogBox::clear(); break;
		default: assert_sentinel;
	}
}

static void resizeMainWindow(size_t cx, size_t cy, bool repaints)
{
	if ( ! g_res ) return;

	int const
		  sourceLineBoxSizeY = 50
		, buttonSizeX = cx / countStepButtons
		, buttonSizeY = 20
		, tvSizeY = cy - (sourceLineBoxSizeY + buttonSizeY)
		;

	MoveWindow(hVarTree
		, 0, 0
		, cx, tvSizeY
		, repaints);
	MoveWindow(hSrcLine
		, 0, tvSizeY
		, cx, sourceLineBoxSizeY
		, repaints);

	for ( size_t i = 0; i < countStepButtons; ++ i ) {
		MoveWindow(g_res->stepButtons[i]
			, i * buttonSizeX
			, tvSizeY + sourceLineBoxSizeY
			, buttonSizeX, buttonSizeY
			, repaints);
	}
}

// メインウィンドウのコールバック関数
LRESULT CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch ( msg ) {
		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
				case IDC_BTN1: Knowbug::run();         break;
				case IDC_BTN2: Knowbug::runStepIn();   break;
				case IDC_BTN3: Knowbug::runStop();     break;
				case IDC_BTN4: Knowbug::runStepOver(); break;
				case IDC_BTN5: Knowbug::runStepOut();  break;

				case IDC_TOPMOST: {
					Menu_ToggleCheck(g_res->dialogMenu.get(), IDC_TOPMOST, g_config->bTopMost);
					for ( auto&& hwnd : windowHandles() ) {
						Window_SetTopMost(hwnd, g_config->bTopMost);
					}
					break;
				}
				case IDC_OPEN_CURRENT_SCRIPT: {
					if ( auto const&& p =
							VTRoot::script()->resolveRefName(g_dbginfo->curFileName())
						) {
						ShellExecute(nullptr, "open"
							, p->c_str(), nullptr, "", SW_SHOWDEFAULT);
					}
					break;
				}
				case IDC_OPEN_INI: {
					std::ofstream of { g_config->selfPath(), std::ios::app }; //create empty file if not exist
					ShellExecute(nullptr, "open"
						, g_config->selfPath().c_str(), nullptr, "", SW_SHOWDEFAULT);
					break;
				}
				case IDC_UPDATE: {
					View::update();
					break;
				}
				case IDC_OPEN_KNOWBUG_REPOS: {
					ShellExecute(nullptr, "open"
						, KnowbugRepoUrl, nullptr, "", SW_SHOWDEFAULT);
					break;
				}
				case IDC_GOTO_LOG: {
					g_res->tv->selectNode(*VTRoot::log());
					break;
				}
				case IDC_GOTO_SCRIPT: {
					g_res->tv->selectNode(*VTRoot::script());
					break;
				}
			}
			break;

		case WM_CONTEXTMENU: {
			if ( (HWND)wp == hVarTree ) {
				POINT pt = { LOWORD(lp), HIWORD(lp) };
				if ( auto&& hItem = TreeView_GetItemAtPoint(hVarTree, pt) ) {
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
					case NM_DBLCLK:
					case NM_RETURN:
					case TVN_SELCHANGED:
						View::update();
						break;
					case TVN_SELCHANGING:
						View::saveCurrentCaret();
						break;
					case NM_CUSTOMDRAW: {
						if ( !g_config->bCustomDraw ) break;
						LRESULT const res =
							g_res->tv->customDraw(reinterpret_cast<LPNMTVCUSTOMDRAW>(nmhdr));
						SetWindowLongPtr(hDlg, DWLP_MSGRESULT, res);
						return TRUE;
					}
				}
			}
			break;
		}
		case WM_SIZE:
			resizeMainWindow(LOWORD(lp), HIWORD(lp), true);
			break;
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

void Dialog::createMain()
{
	int const dispx = GetSystemMetrics(SM_CXSCREEN);
	int const dispy = GetSystemMetrics(SM_CYSCREEN);

	int const mainSizeX = 234, mainSizeY = 380;
	int const viewSizeX = g_config->viewSizeX, viewSizeY = g_config->viewSizeY;

	//ビューウィンドウ
	window_handle_t hViewWnd {
		Window_Create
			( "KnowbugViewWindow", ViewDialogProc
			, KnowbugViewWindowTitle, (WS_THICKFRAME)
			, viewSizeX, viewSizeY
			, dispx - mainSizeX - viewSizeX, 0
			, Knowbug::getInstance()
			) };
	SetWindowLongPtr(hViewWnd.get(), GWL_EXSTYLE
		, GetWindowLongPtr(hViewWnd.get(), GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
	{
		HWND const hPane =
			CreateDialog(Knowbug::getInstance()
				, (LPCSTR)IDD_VIEW_PANE
				, hViewWnd.get(), (DLGPROC)ViewDialogProc);
		hViewEdit = GetDlgItem(hPane, IDC_VIEW);
		setEditStyle(hViewEdit, g_config->maxLength);

		ShowWindow(hPane, SW_SHOW);
	}

	//メインウィンドウ
	window_handle_t hDlgWnd {
		Window_Create
			( "KnowbugMainWindow", DlgProc
			, KnowbugMainWindowTitle, WS_THICKFRAME
			, mainSizeX, mainSizeY
			, dispx - mainSizeX, 0
			, Knowbug::getInstance()
			) };
	{
		HWND const hPane =
			CreateDialog(Knowbug::getInstance()
				, (LPCSTR)IDD_MAIN_PANE
				, hDlgWnd.get(), (DLGPROC)DlgProc);
		ShowWindow(hPane, SW_SHOW);

		//メニューバー
		menu_handle_t hDlgMenu { LoadMenu(Knowbug::getInstance(), (LPCSTR)IDR_MAIN_MENU) };
		SetMenu(hDlgWnd.get(), hDlgMenu.get());

		//ポップメニュー
		HMENU const hNodeMenuBar = LoadMenu(Knowbug::getInstance(), (LPCSTR)IDR_NODE_MENU);

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
			, std::make_unique<VTView>()
			, {{
				  GetDlgItem(hPane, IDC_BTN1)
				, GetDlgItem(hPane, IDC_BTN2)
				, GetDlgItem(hPane, IDC_BTN3)
				, GetDlgItem(hPane, IDC_BTN4)
				, GetDlgItem(hPane, IDC_BTN5) }}
			, gdi_obj_t {
					Font_Create
						( g_config->fontFamily.c_str()
						, g_config->fontSize
						, g_config->fontAntialias ) }
			});
	}

	if ( g_config->bTopMost ) {
		CheckMenuItem(g_res->dialogMenu.get(), IDC_TOPMOST, MF_CHECKED);
		for ( auto&& hwnd : windowHandles() ) {
			Window_SetTopMost(hwnd, true);
		}
	}
	if ( g_config->scrollsLogAutomatically ) {
		CheckMenuItem(g_res->logMenu.get(), IDC_LOG_AUTO_SCROLL, MF_CHECKED);
	}
	if ( g_config->logsInvocation ) {
		CheckMenuItem(g_res->logMenu.get(), IDC_LOG_INVOCATION, MF_CHECKED);
	}

	for ( auto&& hwnd : { hSrcLine, hViewEdit } ) {
		SendMessage(hwnd, WM_SETFONT
			, (WPARAM)(g_res->font.get())
			, /* repaints = */ FALSE);
	}

	{
		RECT rc; GetClientRect(g_res->mainWindow.get(), &rc);
		resizeMainWindow(rc.right, rc.bottom, false);
	}
	{
		RECT rc; GetClientRect(g_res->viewWindow.get(), &rc);
		MoveWindow(hViewEdit, 0, 0, rc.right, rc.bottom, FALSE);
	}

	for ( auto&& hwnd : windowHandles() ) {
		UpdateWindow(hwnd);
		ShowWindow(hwnd, SW_SHOW);
	}
}

void Dialog::destroyMain()
{
	g_res.reset();
}

// 一時停止時に dbgnotice から呼ばれる
void update()
{
	CurrentUpdate();
	g_res->tv->update();
}

void setEditStyle( HWND hEdit, int maxlen )
{
	Edit_SetTabLength(hEdit, g_config->tabwidth);
	SendMessage(hEdit, EM_SETLIMITTEXT, (WPARAM)maxlen, 0);
}

} // namespace Dialog

// 公開API

EXPORT HWND WINAPI knowbug_hwnd()
{
	return Dialog::g_res->mainWindow.get();
}

EXPORT HWND WINAPI knowbug_hwndView()
{
	return Dialog::g_res->viewWindow.get();
}
