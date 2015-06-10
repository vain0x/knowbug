
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
#include "WrapCall/WrapCall.h"

#include "dialog.h"
#include "vartree.h"
#include "config_mng.h"
#include "DebugInfo.h"

#define KnowbugAppName "Knowbug"
#define KnowbugVersion "1.0b19"

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

HWND getKnowbugHandle() { return hDlgWnd; }
HWND getSttCtrlHandle() { return hSttCtrl; }
HWND getVarTreeHandle() { return hVarTree; }

static void setEditStyle(HWND hEdit, int maxlen);

//------------------------------------------------
// ビュー更新
//------------------------------------------------
void UpdateView()
{
	HTREEITEM const hItem = TreeView_GetSelection(hVarTree);
	if ( hItem ) {
		string const varinfoText = VarTree::getItemVarText(hItem);
		if ( !varinfoText.empty() ) {
			Edit_UpdateText(hViewEdit, varinfoText.c_str());
		}
	}
}

//------------------------------------------------
// ログのチェックボックス
//------------------------------------------------
bool updatesLogAutomatically()
{
	return g_config->updatesLogAutomatically;
}

bool logsCalling()
{
	return g_config->logsInvocation;
}

//------------------------------------------------
// ログボックス
//------------------------------------------------
namespace LogBox {
	static HWND hwnd_;
	static string stock_;
	static string buf_;

	void init(HWND hwnd) {
		hwnd_ = hwnd;
	}
	string const& get() {
		return buf_;
	}
	void clearImpl() {
		stock_.clear();
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
	void update() {
		commit(stock_.c_str());
		stock_.clear();
	}
	void add(char const* str) {
		if ( !str || str[0] == '\0' ) return;
		if ( updatesLogAutomatically() ) {
			commit(str);
		} else {
			stock_.append(str);
		}
	}
	void save(char const* filepath) {
		std::ofstream ofs(filepath);
		if ( ofs.is_open() ) {
			ofs.write(buf_.c_str(), buf_.size());
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
#include "module/LineDelimitedString.h"
using script_t = LineDelimitedString;

// 現在ソースタブで表示されているファイルのパス
static string stt_viewingFilepath;

// 読み込み処理 (failure: nullptr)
static script_t const* ReadFromSourceFile(char const* _filepath)
{
	string const filepath = _filepath;

	// キャッシュから検索
	static std::map<string const, script_t> stt_cache;
	{
		auto const iter = stt_cache.find(filepath);
		if ( iter != stt_cache.end() ) return &iter->second;
	}

	// ファイルから読み込む
	std::ifstream ifs { filepath };
	if ( !ifs.is_open() ) { //search "common" folder
		char path[MAX_PATH];
		if ( SearchPath(g_config->commonPath().c_str(), _filepath, nullptr, sizeof(path), path, nullptr) == 0 ) {
			return nullptr;
		}
		ifs.open(path);
		if ( !ifs.is_open() ) return nullptr;
	}
	auto const res =
		stt_cache.emplace(filepath, ifs);
	return &res.first->second;
}

// ソースタブを同期する
static void SrcSync(char const* filepath, int line_num, bool bUpdateEdit, bool bUpdateBox)
{
	if ( !filepath || line_num < 0 ) return;
	auto const&& curinf = DebugInfo::formatCurInfString(filepath, line_num);

	if ( auto const p = ReadFromSourceFile(filepath) ) {
		assert(line_num >= 1);	// 行番号 line_num は 1-based
		size_t const iLine = static_cast<size_t>(line_num - 1);
		auto&& ran = p->lineRange(iLine);

#if 0
		if ( bUpdateEdit ) {
			if ( stt_viewingFilepath != filepath ) {
				SrcSyncImpl(hSrcEdit, p->get().c_str());
				stt_viewingFilepath = filepath;
			}
			Edit_SetSel(hSrcEdit, ran.first, ran.second); // 該当行を選択
			Edit_Scroll(hSrcEdit, iLine, 0);
		}
#endif
		if ( bUpdateBox ) {
			Edit_UpdateText(hSrcLine, (curinf + "\r\n" + p->line(iLine)).c_str());
		}
	} else {
		//if ( bUpdateEdit ) Edit_UpdateText(hSrcEdit, text.c_str());
		if ( bUpdateBox ) Edit_UpdateText(hSrcLine, curinf.c_str());
	}
}

string const* tryGetCurrentScript() {
	if ( auto const p = ReadFromSourceFile(g_dbginfo->debug->fname) ) {
		return &p->get();
	}
	return nullptr;
}

//------------------------------------------------
// ソースタブの更新
//------------------------------------------------
static void TabSrcUpdate()
{
	SrcSync(g_dbginfo->debug->fname, g_dbginfo->debug->line, true, false);
}

//------------------------------------------------
// 実行中の位置表示を更新する (line, file)
//------------------------------------------------
static void CurrentUpdate()
{
	SrcSync(g_dbginfo->debug->fname, g_dbginfo->debug->line, false, true);
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
					g_config->bTopMost = !g_config->bTopMost;
					CheckMenuItem(hDlgMenu, IDC_TOPMOST, (g_config->bTopMost ? MF_CHECKED : MF_UNCHECKED));
					Window_SetTopMost(hDlgWnd, g_config->bTopMost);
					Window_SetTopMost(hViewWnd, g_config->bTopMost);
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
			// ツリー上で逆クリック
			if ( (HWND)wp == hVarTree ) {
				TV_HITTESTINFO tvHitTestInfo;
				tvHitTestInfo.pt = { LOWORD(lp), HIWORD(lp) };
				ScreenToClient(hVarTree, &tvHitTestInfo.pt);
				auto const hItem = TreeView_HitTest(hVarTree, &tvHitTestInfo);
				if ( !hItem ) break;

				break; //unsupported

				if ( tvHitTestInfo.flags & TVHT_ONITEMLABEL ) {		// 文字列アイテムの場合
					auto const nodeString = TreeView_GetItemString(hVarTree, hItem);
					HMENU hPop;
					if ( VarTree::InvokeNode::isTypeOf(nodeString.c_str()) ) {
						hPop = hInvokeNodeMenu;
					} else if ( VarTree::TreeView_MyLParam<VarTree::SystemNode>(hVarTree, hItem) == VarTree::SystemNodeId::Log ) {
						hPop = hLogNodeMenu;
					} else {
						hPop = hNodeMenu;
					}
					// ポップアップメニューを表示する
					int const idSelected = TrackPopupMenuEx(
						hPop, (TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD),
						(int)LOWORD(lp), (int)HIWORD(lp), hDlgWnd, nullptr
						);

					switch ( idSelected ) {
						case IDC_NODE_UPDATE: UpdateView(); break;
						case IDC_NODE_LOG: {
							string const&& varinfoText = VarTree::getItemVarText(hItem);
							Knowbug::logmes(varinfoText.c_str());
							return TRUE;
						}
						case IDC_NODE_STEP_OUT: {
							assert(VarTree::InvokeNode::isTypeOf(nodeString.c_str()));
							auto const idx = VarTree::TreeView_MyLParam<VarTree::InvokeNode>(hVarTree, hItem);
							if ( auto const pCallInfo = WrapCall::getCallInfoAt(idx) ) {
								// 対象が呼び出された階層まで進む
								Knowbug::runStepReturn(pCallInfo->sublev);
							}
							break;
						}
						case IDC_LOG_AUTO_UPDATE: break; //自動更新のチェックを反転
						case IDC_LOG_INVOCATION: break; //呼び出しをログするかのチェックを反転
						case IDC_LOG_SAVE: LogBox::save(); break;
						case IDC_LOG_CLEAR: LogBox::clear(); break;
						default: break;
					}
					return TRUE;
				}
			}
			break;
		}
		case WM_NOTIFY: {
			auto const nmhdr = reinterpret_cast<LPNMHDR>(lp);
			if ( nmhdr->hwndFrom == hVarTree ) {
				switch ( nmhdr->code ) {
					case TVN_SELCHANGED:
					case NM_DBLCLK:
					case NM_RETURN:
						UpdateView();
						break;

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
	int const viewSizeX = 300, viewSizeY = mainSizeY;

	//ビューウィンドウ
	hViewWnd = MyCreateWindow("KnowbugViewWindow", ViewDialogProc, "Knowbug View", (WS_THICKFRAME),
		viewSizeX, viewSizeY,
		dispx - mainSizeX - viewSizeX, 0
	);
	SetWindowLong(hViewWnd, GWL_EXSTYLE, GetWindowLongPtr(hViewWnd, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
	{
		HWND const hPane = CreateDialog(Knowbug::getInstance(), (LPCSTR)IDD_VIEW_PANE, hViewWnd, (DLGPROC)ViewDialogProc);
		hViewEdit = GetDlgItem(hPane, IDC_VIEW);
		setEditStyle(hViewEdit, g_config->maxlenVarinfo);

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
