
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

#define KnowbugAppName "Knowbug"
#define KnowbugVersion "1.0b20"

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
static std::map<HTREEITEM, shared_ptr<string const>> vartree_textCache; //一回の停止中にのみ有効

HWND getKnowbugHandle() { return hDlgWnd; }
HWND getSttCtrlHandle() { return hSttCtrl; }
HWND getVarTreeHandle() { return hVarTree; }

static LineDelimitedString const* ReadFromSourceFile(char const* _filepath);
static void setEditStyle(HWND hEdit, int maxlen);

//------------------------------------------------
// ノード文字列の取得 (memoized)
//------------------------------------------------
static string const& getVarNodeString(HTREEITEM hItem)
{
	auto&& stringPtr = map_find_or_insert(vartree_textCache, hItem, [&hItem]() {
		return VarTree::getItemVarText(hItem);
	});
	assert(stringPtr);
	return *stringPtr;
}

//------------------------------------------------
// ビューキャレット位置を変更
//------------------------------------------------
static void SaveViewCaret()
{
	HTREEITEM const hItem = TreeView_GetSelection(hVarTree);
	if ( hItem != nullptr ) {
		int const vcaret = Edit_GetFirstVisibleLine(hViewEdit);
		if ( vcaret != 0 ) {
			vartree_vcaret[hItem] = vcaret;
		}
	}
}

//------------------------------------------------
// ビュー更新
//------------------------------------------------
static void UpdateView()
{
	HTREEITEM const hItem = TreeView_GetSelection(hVarTree);
	if ( hItem ) {
		string const& varinfoText = getVarNodeString(hItem);
		SetWindowText(hViewEdit, varinfoText.c_str());

		//+script ノードなら現在の実行位置を選択
		if ( hItem == VarTree::getScriptNodeHandle() ) {
			auto const p = ReadFromSourceFile(g_dbginfo->curFileName());
			int const iLine = g_dbginfo->curLine();
			Edit_Scroll(hViewEdit, std::max(0, iLine - 3), 0);
			Edit_SetSel(hViewEdit, Edit_LineIndex(hViewEdit, iLine), Edit_LineIndex(hViewEdit, iLine + 1));

		//+log ノードの自動スクロール
		} else if ( hItem == VarTree::getLogNodeHandle() && g_config->scrollsLogAutomatically ) {
			Edit_Scroll(hViewEdit, Edit_GetLineCount(hViewEdit), 0);

		} else {
			auto&& it = vartree_vcaret.find(hItem);
			int const vcaret = (it != vartree_vcaret.end() ? it->second : 0);
			Edit_Scroll(hViewEdit, vcaret, 0);
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
	static HWND hwnd_;
	static string buf_;

	void init(HWND hwnd) {
		hwnd_ = hwnd;
	}
	string const& get() {
		return buf_;
	}
	void clearImpl() {
		buf_.clear();
	}
	void clear() {
		if ( !g_config->warnsBeforeClearingLog
			|| MessageBox(hDlgWnd, "ログをすべて消去しますか？", KnowbugAppName, MB_OKCANCEL) == IDOK ) {
			clearImpl();
		}
	}
	void commit(char const* textAdd) {
		buf_ += textAdd;
	}
	void add(char const* str) {
		if ( !str || str[0] == '\0' ) return;
		commit(str);
	}
	void save(char const* filepath) {
		std::ofstream ofs(filepath);
		ofs.write(buf_.c_str(), buf_.size());
		if ( ofs.bad() ) {
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
// ソースファイルを開く
//
// @ エディタ上で編集中の場合、ファイルの内容が実際と異なることがある。行番号のアウトレンジに注意。
//------------------------------------------------

static std::unique_ptr<string> TrySearchFile(char const* filepath) {
	char* filename = nullptr;
	char fullpath[MAX_PATH + 2] {};
	if ( SearchPath(nullptr, filepath, nullptr, sizeof(fullpath), fullpath, &filename) ) {
		return std::make_unique<string>(fullpath);
	} else if ( SearchPath(g_config->commonPath().c_str(), filepath, nullptr, sizeof(fullpath), fullpath, &filename) ) {
		return std::make_unique<string>(fullpath);
	}
	return nullptr;
}

// 読み込み処理
optional_ref<LineDelimitedString const> ReadFromSourceFile(char const* _filepath)
{
	if ( auto const&& p = TrySearchFile(_filepath) ) {
		string const filepath = *p;

		// キャッシュから検索
		static std::map<string const, LineDelimitedString> stt_cache;
		auto& lds = map_find_or_insert(stt_cache, filepath, [&filepath]() {
			std::ifstream ifs { filepath };
			assert(ifs.is_open());
			return LineDelimitedString(ifs);
		});
		return &lds;
	}
	return nullptr;
}

// ソースタブを同期する
static void UpdateCurInfEdit(char const* filepath, int iLine)
{
	if ( !filepath || iLine < 0 ) return;
	auto const&& curinf = DebugInfo::formatCurInfString(filepath, iLine);

	if ( auto const p = ReadFromSourceFile(filepath) ) {
		SetWindowText(hSrcLine, (curinf + "\r\n" + p->line(iLine)).c_str());

	} else {
		SetWindowText(hSrcLine, curinf.c_str());
	}
}

optional_ref<string const> tryGetCurrentScript() {
	if ( auto const p = ReadFromSourceFile(g_dbginfo->curFileName()) ) {
		return &p->get();
	}
	return nullptr;
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
void VarTree_PopupMenu(HTREEITEM hItem, int x, int y)
{
	auto const nodeString = TreeView_GetItemString(hVarTree, hItem);
	HMENU hPop;
#ifdef with_WrapCall
	if ( VarTree::InvokeNode::isTypeOf(nodeString.c_str()) ) {
		hPop = hInvokeNodeMenu;
	} else
#endif //defined(with_WrapCall)
	if ( VarTree::SystemNode::isTypeOf(nodeString.c_str())
		&& VarTree::TreeView_MyLParam<VarTree::SystemNode>(hVarTree, hItem) == VarTree::SystemNodeId::Log ) {
		hPop = hLogNodeMenu;
	} else {
		hPop = hNodeMenu;
	}

	// ポップアップメニューを表示する
	int const idSelected = TrackPopupMenuEx(
		hPop, (TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD),
		x, y, hDlgWnd, nullptr
	);

	switch ( idSelected ) {
		case 0: break;
		case IDC_NODE_UPDATE: UpdateView(); break;
		case IDC_NODE_LOG: {
			Knowbug::logmes(getVarNodeString(hItem).c_str());
			break;
		}
#ifdef with_WrapCall
		case IDC_NODE_STEP_OUT: {
			auto const idx = VarTree::TreeView_MyLParam<VarTree::InvokeNode>(hVarTree, hItem);
			if ( auto const pCallInfo = WrapCall::tryGetCallInfoAt(idx) ) {
				// 対象が呼び出された階層まで進む
				Knowbug::runStepReturn(pCallInfo->sublev);
			}
			break;
		}
#endif //defined(with_WrapCall)
		case IDC_LOG_AUTO_SCROLL: {
			bool& b = g_config->scrollsLogAutomatically;
			b = !b;
			CheckMenuItem(hLogNodeMenu, IDC_LOG_AUTO_SCROLL, (b ? MF_CHECKED : MF_UNCHECKED));
			break;
		}
		case IDC_LOG_INVOCATION: {
			bool& b = g_config->logsInvocation;
			b = !b;
			CheckMenuItem(hLogNodeMenu, IDC_LOG_INVOCATION, (b ? MF_CHECKED : MF_UNCHECKED));
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
					bool& b = g_config->bTopMost;
					b = !b;
					CheckMenuItem(hDlgMenu, IDC_TOPMOST, (b ? MF_CHECKED : MF_UNCHECKED));
					Window_SetTopMost(hDlgWnd, g_config->bTopMost);
					Window_SetTopMost(hViewWnd, g_config->bTopMost);
					break;
				}
				case IDC_OPEN_CURRENT_SCRIPT: {
					if ( auto const&& p = TrySearchFile(g_dbginfo->curFileName()) ) {
						ShellExecute(nullptr, "open", p->c_str(), nullptr, "", SW_SHOWDEFAULT);
					}
					break;
				}
				case IDC_OPEN_INI: {
					std::ofstream of { g_config->selfPath(), std::ios::app }; //create empty file if not exist
					ShellExecute(nullptr, "open", g_config->selfPath().c_str(), nullptr, "", SW_SHOWDEFAULT);
					break;
				}
				case IDC_HELP_VER: {
					MessageBox(nullptr, strf("%s ver%s", KnowbugAppName, KnowbugVersion).c_str(), KnowbugAppName, MB_OK);
					break;
				}
				case IDC_HELP_DEVREPOS: {
					ShellExecute(nullptr, "open", "https://github.com/vain0/knowbug", nullptr, "", SW_SHOWDEFAULT);
					break;
				}
			}
			break;

		case WM_CONTEXTMENU: {
			if ( (HWND)wp == hVarTree ) {
				TV_HITTESTINFO tvHitTestInfo;
				tvHitTestInfo.pt = { LOWORD(lp), HIWORD(lp) };
				ScreenToClient(hVarTree, &tvHitTestInfo.pt);
				auto const hItem = TreeView_HitTest(hVarTree, &tvHitTestInfo);
				if ( hItem && tvHitTestInfo.flags & TVHT_ONITEMLABEL ) { //文字列アイテムにヒット
					VarTree_PopupMenu(hItem, LOWORD(lp), HIWORD(lp));
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
						SetWindowLong(hDlg, DWL_MSGRESULT, res);
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
// 簡易ウィンドウ生成
//------------------------------------------------
static HWND MyCreateWindow(char const* className, WNDPROC proc, char const* caption, int windowStyles, int sizeX, int sizeY, int posX, int posY)
{
	WNDCLASS wndclass;
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = proc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = Knowbug::getInstance();
	wndclass.hIcon         = nullptr;
	wndclass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndclass.lpszMenuName  = nullptr;
	wndclass.lpszClassName = className;
	RegisterClass(&wndclass);
	
	HWND hWnd = CreateWindow(className, caption,
		(WS_CAPTION | WS_VISIBLE | windowStyles),
		posX, posY, sizeX, sizeY,
		nullptr, nullptr,
		Knowbug::getInstance(),
		nullptr
	);
	if ( !hWnd ) {
		MessageBox(nullptr, "Debug window initalizing failed.", "Error", 0);
		abort();
	}
	return hWnd;
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
	hViewWnd = MyCreateWindow("KnowbugViewWindow", ViewDialogProc, "Knowbug View", (WS_THICKFRAME),
		viewSizeX, viewSizeY,
		dispx - mainSizeX - viewSizeX, 0
	);
	SetWindowLong(hViewWnd, GWL_EXSTYLE, GetWindowLongPtr(hViewWnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
	{
		HWND const hPane = CreateDialog(Knowbug::getInstance(), (LPCSTR)IDD_VIEW_PANE, hViewWnd, (DLGPROC)ViewDialogProc);
		hViewEdit = GetDlgItem(hPane, IDC_VIEW);
		setEditStyle(hViewEdit, g_config->maxLength);

		//エディタをクライアント領域全体に広げる
		RECT rc; GetClientRect(hViewWnd, &rc);
		MoveWindow(hViewEdit, 0, 0, rc.right, rc.bottom, false);

		ShowWindow(hPane, SW_SHOW);
	}

	//メインウィンドウ
	hDlgWnd = MyCreateWindow("KnowbugMainWindow", DlgProc, "Debug Window", 0x0000,
		mainSizeX, mainSizeY,
		dispx - mainSizeX, 0
	);
	{
		HWND const hPane = CreateDialog(Knowbug::getInstance(), (LPCSTR)IDD_MAIN_PANE, hDlgWnd, (DLGPROC)DlgProc);
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
	}

	if ( g_config->bTopMost ) {
		CheckMenuItem(hDlgMenu, IDC_TOPMOST, MF_CHECKED);
		Window_SetTopMost(hDlgWnd, true);
		Window_SetTopMost(hViewWnd, true);
	}
	if ( g_config->scrollsLogAutomatically ) {
		CheckMenuItem(hLogNodeMenu, IDC_LOG_AUTO_SCROLL, MF_CHECKED);
	}
	if ( g_config->logsInvocation ) {
		CheckMenuItem(hLogNodeMenu, IDC_LOG_INVOCATION, MF_CHECKED);
	}

	UpdateWindow(hDlgWnd); ShowWindow(hDlgWnd, SW_SHOW);
	UpdateWindow(hViewWnd); ShowWindow(hViewWnd, SW_SHOW);
}

void Dialog::destroyMain()
{
	if ( !g_config->logPath.empty() ) { //auto save
		LogBox::save(g_config->logPath.c_str());
	}

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
	vartree_textCache.clear();

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
