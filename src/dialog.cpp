
#pragma comment(lib, "comctl32.lib")

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <map>
#include <vector>
#include <algorithm>
#include <array>
#include <fstream>

#include "main.h"
#include "resource.h"
#include "hspwnd.h"
#include "module/supio/supio.h"
#include "module/GuiUtility.h"
#include "module/LineDelimitedString.h"
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

namespace Dialog
{

static int const TABDLGMAX = 4;
static int const CountStepButtons = 5;

static HWND hDlgWnd;
static std::array<HWND, CountStepButtons> hStepButtons;
static HWND hSttCtrl;
static HWND hVarTree;
static HWND hSrcLine;

static HWND hViewWnd;
static HWND hViewEdit;

static HMENU hDlgMenu, hNodeMenu, hLogNodeMenu, hInvokeNodeMenu;

static std::map<HTREEITEM, int> vartree_vcaret;

HWND getKnowbugHandle() { return hDlgWnd; }
HWND getSttCtrlHandle() { return hSttCtrl; }
HWND getVarTreeHandle() { return hVarTree; }

static auto windowHandles() -> std::vector<HWND>
{
	return std::vector<HWND> { hDlgWnd, hViewWnd };
}
static void setEditStyle(HWND hEdit, int maxlen);

//------------------------------------------------
// ビューキャレット位置を変更
//------------------------------------------------
static void SaveViewCaret()
{
	HTREEITEM const hItem = TreeView_GetSelection(hVarTree);
	if ( hItem != nullptr ) {
		int const vcaret = Edit_GetFirstVisibleLine(hViewEdit);
		vartree_vcaret[hItem] = vcaret;
	}
}

//------------------------------------------------
// ビュー更新
//------------------------------------------------
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
	Edit_SetSel(hViewEdit, Edit_LineIndex(hViewEdit, index), Edit_LineIndex(hViewEdit, index + 1));
}

} // namespace View

static void UpdateView()
{
	HTREEITEM const hItem = TreeView_GetSelection(hVarTree);
	if ( hItem ) {
		static HTREEITEM stt_prevSelection = nullptr;
		if ( hItem == stt_prevSelection ) {
			SaveViewCaret();
		} else {
			stt_prevSelection = hItem;
		}

		std::shared_ptr<string const> varinfoText = VarTree::getItemVarText(hItem);
		Dialog::View::setText(varinfoText->c_str());

		//+script ノードなら現在の実行位置を選択
		if ( hItem == VarTree::getScriptNodeHandle() ) {
			int const iLine = g_dbginfo->curLine();
			Dialog::View::scroll(std::max(0, iLine - 3), 0);
			Dialog::View::selectLine(iLine);

		//+log ノードの自動スクロール
		} else if ( hItem == VarTree::getLogNodeHandle() && g_config->scrollsLogAutomatically ) {
			Dialog::View::scrollBottom();

		} else {
			auto&& it = vartree_vcaret.find(hItem);
			int const vcaret = (it != vartree_vcaret.end() ? it->second : 0);
			Dialog::View::scroll(vcaret, 0);
		}
	}
}

//------------------------------------------------
// ログのチェックボックス
//------------------------------------------------
bool logsCalling()
{
	return g_config->logsInvocation;
}

//------------------------------------------------
// ログボックス
//------------------------------------------------
namespace LogBox {
	void clear()
	{
		if ( !g_config->warnsBeforeClearingLog
			|| MessageBox(hDlgWnd, "ログをすべて消去しますか？", KnowbugAppName, MB_OKCANCEL) == IDOK ) {
			VTRoot::log()->clear();
		}
	}

	struct LogBoxObserver : VTNodeLog::LogObserver
	{
		void afterAppend(char const* addition) override
		{
			//キャッシュを消して更新
			VarTree::eraseTextCache(VarTree::getLogNodeHandle());
			if ( TreeView_GetSelection(hVarTree) == VarTree::getLogNodeHandle() ) {
				UpdateView();
			}
		}
	};

	void init()
	{
		VTRoot::log()->setLogObserver(std::make_shared<LogBoxObserver>());
	}

	void save(char const* filepath) {
		if ( !VTRoot::log()->save(filepath) ) {
			MessageBox(hDlgWnd, "ログの保存に失敗しました。", KnowbugAppName, MB_OK);
		}
	}
	void save() {
		char filename[MAX_PATH + 1] = "";
		char fullname[MAX_PATH + 1] = "hspdbg.log";
		OPENFILENAME ofn = { 0 };
			ofn.lStructSize    = sizeof(ofn);         // 構造体のサイズ
			ofn.hwndOwner      = hDlgWnd;             // コモンダイアログの親ウィンドウハンドル
			ofn.lpstrFilter    = "log text(*.txt;*.log)\0*.txt;*.log\0All files(*.*)\0*.*\0\0";	// ファイルの種類
			ofn.lpstrFile      = fullname;            // 選択されたファイル名(フルパス)を受け取る変数のアドレス
			ofn.lpstrFileTitle = filename;            // 選択されたファイル名を受け取る変数のアドレス
			ofn.nMaxFile       = sizeof(fullname);    // lpstrFileに指定した変数のサイズ
			ofn.nMaxFileTitle  = sizeof(filename);    // lpstrFileTitleに指定した変数のサイズ
			ofn.Flags          = OFN_OVERWRITEPROMPT; // フラグ指定
			ofn.lpstrTitle     = "名前を付けて保存";   // コモンダイアログのキャプション
			ofn.lpstrDefExt    = "log";               // デフォルトのファイルの種類

		if ( GetSaveFileName(&ofn) ) {
			save(fullname);
		}
	}
} //namespace LogBox

//------------------------------------------------
// ソースタブを同期する
//------------------------------------------------
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

//------------------------------------------------
// 実行中の位置表示を更新する (line, file)
//------------------------------------------------
static void CurrentUpdate()
{
	UpdateCurInfEdit(g_dbginfo->curFileName(), g_dbginfo->curLine());
}

//------------------------------------------------
// ツリーノードのコンテキストメニュー
//------------------------------------------------
void VarTree_PopupMenu(HTREEITEM hItem, POINT pt)
{
	struct GetPopMenu
		: public VTNodeData::Visitor
	{
		void fInvoke(VTNodeInvoke const&) override
		{
			hPop = hInvokeNodeMenu;
		}
		void fLog(VTNodeLog const&) override
		{
			hPop = hLogNodeMenu;
		}
		HMENU apply(VTNodeData const& node)
		{
			hPop = hNodeMenu; // default
			node.acceptVisitor(*this);
			return hPop;
		}
	private:
		HMENU hPop;
	};

	auto&& node = VarTree::tryGetNodeData(hItem);
	if ( !node ) return;
	HMENU const hPop = GetPopMenu {}.apply(*node);

	// ポップアップメニューを表示する
	int const idSelected =
		TrackPopupMenuEx
			( hPop, (TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD)
			, pt.x, pt.y, hDlgWnd, nullptr);

	switch ( idSelected ) {
		case 0: break;
		case IDC_NODE_UPDATE: UpdateView(); break;
		case IDC_NODE_LOG: {
			Knowbug::logmes(VarTree::getItemVarText(hItem)->c_str());
			break;
		}
#ifdef with_WrapCall
		case IDC_NODE_STEP_OUT: {
			if ( auto&& nodeInvoke = std::dynamic_pointer_cast<VTNodeInvoke const>(node) ) {
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

//------------------------------------------------
// 親ダイアログのコールバック関数
//------------------------------------------------
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
					Menu_ToggleCheck(hDlgMenu, IDC_TOPMOST, g_config->bTopMost);
					for ( auto&& hwnd : windowHandles() ) {
						Window_SetTopMost(hwnd, g_config->bTopMost);
					}
					break;
				}
				case IDC_OPEN_CURRENT_SCRIPT: {
					if ( auto const&& p = VTRoot::script()->resolveRefName(g_dbginfo->curFileName()) ) {
						ShellExecute(nullptr, "open", p->c_str(), nullptr, "", SW_SHOWDEFAULT);
					}
					break;
				}
				case IDC_OPEN_INI: {
					std::ofstream of { g_config->selfPath(), std::ios::app }; //create empty file if not exist
					ShellExecute(nullptr, "open", g_config->selfPath().c_str(), nullptr, "", SW_SHOWDEFAULT);
					break;
				}
				case IDC_UPDATE: {
					UpdateView();
					break;
				}
				case IDC_OPEN_KNOWBUG_REPOS: {
					ShellExecute(nullptr, "open", "https://github.com/vain0/knowbug", nullptr, "", SW_SHOWDEFAULT);
					break;
				}
				case IDC_GOTO_LOG: {
					TreeView_SelectItem(hVarTree, VarTree::getLogNodeHandle());
					break;
				}
				case IDC_GOTO_SCRIPT: {
					TreeView_SelectItem(hVarTree, VarTree::getScriptNodeHandle());
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
					case TVN_SELCHANGED: UpdateView(); break;
					case TVN_SELCHANGING: SaveViewCaret(); break;
					case TVN_DELETEITEM: {
						NMTREEVIEW* const nmtv = reinterpret_cast<NMTREEVIEW*>(lp);
						vartree_vcaret.erase(nmtv->itemOld.hItem);
						break;
					}
					case NM_CUSTOMDRAW: {
						if ( !g_config->bCustomDraw ) break;
						LRESULT const res = VarTree::customDraw(reinterpret_cast<LPNMTVCUSTOMDRAW>(nmhdr));
						SetWindowLongPtr(hDlg, DWLP_MSGRESULT, res);
						return TRUE;
					}
				}
			}
			break;
		}
		case WM_CREATE: return TRUE;
		case WM_CLOSE: return FALSE;
		case WM_DESTROY:
			DestroyMenu(hNodeMenu);
			DestroyMenu(hLogNodeMenu);
			DestroyMenu(hInvokeNodeMenu);
			DestroyWindow(hViewWnd);
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
		case WM_SIZING: {
			RECT rc; GetClientRect(hDlg, &rc);
			MoveWindow(hViewEdit, 0, 0, rc.right, rc.bottom, false);
			break;
		}
	}
	return DefWindowProc(hDlg, msg, wp, lp);
}

//------------------------------------------------
// メインダイアログを生成する
//------------------------------------------------
void Dialog::createMain()
{
	int const dispx = GetSystemMetrics(SM_CXSCREEN);
	int const dispy = GetSystemMetrics(SM_CYSCREEN);

	int const mainSizeX = 234, mainSizeY = 380;
	int const viewSizeX = g_config->viewSizeX, viewSizeY = g_config->viewSizeY;

	//ビューウィンドウ
	hViewWnd =
		Window_Create
			( "KnowbugViewWindow", ViewDialogProc
			, KnowbugViewWindowTitle, (WS_THICKFRAME)
			, viewSizeX, viewSizeY
			, dispx - mainSizeX - viewSizeX, 0
			, Knowbug::getInstance());
	SetWindowLongPtr(hViewWnd, GWL_EXSTYLE
		, GetWindowLongPtr(hViewWnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
	{
		HWND const hPane =
			CreateDialog(Knowbug::getInstance()
				, (LPCSTR)IDD_VIEW_PANE
				, hViewWnd, (DLGPROC)ViewDialogProc);
		hViewEdit = GetDlgItem(hPane, IDC_VIEW);
		setEditStyle(hViewEdit, g_config->maxLength);

		//エディタをクライアント領域全体に広げる
		RECT rc; GetClientRect(hViewWnd, &rc);
		MoveWindow(hViewEdit, 0, 0, rc.right, rc.bottom, false);

		ShowWindow(hPane, SW_SHOW);
	}

	//メインウィンドウ
	hDlgWnd =
		Window_Create
			( "KnowbugMainWindow", DlgProc
			, KnowbugMainWindowTitle, 0x0000
			, mainSizeX, mainSizeY
			, dispx - mainSizeX, 0
			, Knowbug::getInstance());
	{
		HWND const hPane =
			CreateDialog(Knowbug::getInstance()
				, (LPCSTR)IDD_MAIN_PANE
				, hDlgWnd, (DLGPROC)DlgProc);
		ShowWindow(hPane, SW_SHOW);

		//メニューバー
		hDlgMenu = LoadMenu(Knowbug::getInstance(), (LPCSTR)IDR_MAIN_MENU);
		SetMenu(hDlgWnd, hDlgMenu);

		//ポップメニュー
		HMENU const hNodeMenuBar = LoadMenu(Knowbug::getInstance(), (LPCSTR)IDR_NODE_MENU);
		hNodeMenu       = GetSubMenu(hNodeMenuBar, 0);
		hInvokeNodeMenu = GetSubMenu(hNodeMenuBar, 1);
		hLogNodeMenu	= GetSubMenu(hNodeMenuBar, 2);

		//いろいろ
		hVarTree = GetDlgItem(hPane, IDC_VARTREE);
		hSrcLine = GetDlgItem(hPane, IDC_SRC_LINE);
		hStepButtons = { {
				GetDlgItem(hPane, IDC_BTN1),
				GetDlgItem(hPane, IDC_BTN2),
				GetDlgItem(hPane, IDC_BTN3),
				GetDlgItem(hPane, IDC_BTN4),
				GetDlgItem(hPane, IDC_BTN5),
			} };

		// ツリービュー
		VarTree::init();

		LogBox::init();
	}

	if ( g_config->bTopMost ) {
		CheckMenuItem(hDlgMenu, IDC_TOPMOST, MF_CHECKED);
		for ( auto&& hwnd : windowHandles() ) {
			Window_SetTopMost(hwnd, true);
		}
	}
	if ( g_config->scrollsLogAutomatically ) {
		CheckMenuItem(hLogNodeMenu, IDC_LOG_AUTO_SCROLL, MF_CHECKED);
	}
	if ( g_config->logsInvocation ) {
		CheckMenuItem(hLogNodeMenu, IDC_LOG_INVOCATION, MF_CHECKED);
	}

	for ( auto&& hwnd : windowHandles() ) {
		UpdateWindow(hwnd);
		ShowWindow(hwnd, SW_SHOW);
	}
}

void Dialog::destroyMain()
{
	VarTree::term();

	if ( hDlgWnd != nullptr ) {
		DestroyWindow(hDlgWnd);
		hDlgWnd = nullptr;
	}
}

//------------------------------------------------
// 更新
// 
// @ dbgnotice (stop) から呼ばれる。
//------------------------------------------------
void update()
{
	VarTree::update();
	CurrentUpdate();
	UpdateView();
}

//------------------------------------------------
// エディットコントロールの標準スタイル
// 
// @ 設定に依存
//------------------------------------------------
void setEditStyle( HWND hEdit, int maxlen )
{
	Edit_SetTabLength(hEdit, g_config->tabwidth);
	SendMessage(hEdit, EM_SETLIMITTEXT, (WPARAM)maxlen, 0);
}

} // namespace Dialog

//##############################################################################
//                公開API
//##############################################################################
EXPORT HWND WINAPI knowbug_hwnd()
{
	return Dialog::getKnowbugHandle();
}

EXPORT HWND WINAPI knowbug_hwndView()
{
	return Dialog::hViewWnd;
}
