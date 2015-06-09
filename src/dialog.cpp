
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
#include "WrapCall/WrapCall.h"

#include "dialog.h"
#include "vartree.h"
#include "config_mng.h"
#include "DebugInfo.h"

#define IDU_TAB 100
#define ID_BTN1 1000
#define ID_BTN2 1001
#define ID_BTN3 1002
#define ID_BTN4 1003
#define ID_BTN5 1004

#define DIALOG_X0 5
#define DIALOG_Y0 5
#define DIALOG_X1 366
#define DIALOG_Y1 406
#define DIALOG_Y2 23

#define WND_SX 380
#define WND_SY 480

namespace Dialog
{

static char const* const myClass = "KNOWBUG";
static int const TABDLGMAX = 4;

static HWND hDlgWnd;

static HWND hTabCtrl;
static HWND hGenList;
static HWND hTabSheet[TABDLGMAX];

static HWND hBtn1;
static HWND hBtn2;
static HWND hBtn3;
static HWND hBtn4;
static HWND hBtn5;
static HWND hSttCtrl;

static HWND hVarPage;
static HWND hVarTree;
static HWND hVarEdit;

static HWND hLogPage;
static HWND hLogChkUpdate;
static HWND hLogChkCalog;

static HWND hSrcPage;
static HWND hSrcEdit;
static HWND hSrcBox;

static HMENU hPopup;
static HMENU hPopupOfVar;

HWND getKnowbugHandle() { return hDlgWnd; }
HWND getSttCtrlHandle() { return hSttCtrl; }
HWND getVarTreeHandle() { return hVarTree; }

static void setEditStyle(HWND hEdit, int maxlen);

//------------------------------------------------
// ウィンドウ・オブジェクトの生成
//------------------------------------------------
static HWND GenerateObj( HWND parent, char const* name, char const* ttl, int x, int y, int sx, int sy, int menu, HFONT font )
{
	HWND const h = CreateWindow(
		name, ttl,
		(WS_CHILD | WS_VISIBLE),
		x, y, sx, sy,
		parent,
		(HMENU)menu,
		Knowbug::getInstance(),
		nullptr
	);

	if ( font ) SendMessage( h, WM_SETFONT, (WPARAM)font, TRUE );
	return h;
}

//------------------------------------------------
// 全般タブの前処理
//------------------------------------------------
static void TabGeneralInit()
{
	LVCOLUMN col;
	col.mask     = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	col.fmt      = LVCFMT_LEFT;
	col.cx       = 120;
	col.pszText  = "項目";
	col.iSubItem = 0;
	ListView_InsertColumn( hGenList , 0 , &col);

	col.cx       = 400;
	col.iSubItem = 1;
	col.pszText  = "内容";
	ListView_InsertColumn( hGenList , 1 , &col);
}

//------------------------------------------------
// 全般タブの更新
//------------------------------------------------
static void SrcSync( char const* filepath, int line_num, bool bUpdateEdit, bool bUpdateBox );
static void TabGeneral_AddItem( char const* sItem, char const* sValue, int iItem );

static void TabGeneralUpdate()
{
	ListView_DeleteAllItems( hGenList );

	int cntItems = 0;
	for ( auto&& kv : g_dbginfo->fetchGeneralInfo() ) {
		TabGeneral_AddItem(kv.first.c_str(), kv.second.c_str(), cntItems); ++cntItems;
	}
	SrcSync( g_dbginfo->debug->fname, g_dbginfo->debug->line, false, true );
}

static void TabGeneral_AddItem( char const* sItem, char const* sValue, int iItem )
{
	LV_ITEM item;
	item.mask     = LVIF_TEXT;
	item.iItem    = iItem;

	item.iSubItem = 0;
	item.pszText  = const_cast<char*>(sItem);
	ListView_InsertItem( hGenList, &item );

	item.iSubItem = 1;
	item.pszText  = const_cast<char*>(sValue);
	ListView_SetItem( hGenList, &item );
}

//------------------------------------------------
// 実行中の位置表示を更新する (line, file)
//------------------------------------------------
static void CurrentUpdate()
{
	SetWindowText( hSttCtrl, g_dbginfo->getCurInfString().c_str() );
}

//------------------------------------------------
// 変数タブの初期化
//------------------------------------------------
void TabVarInit( HWND hDlg )
{
	hVarPage = hDlg;
	hVarTree = GetDlgItem( hDlg, IDC_VARTREE );
	hVarEdit = GetDlgItem( hDlg, IDC_VARINFO );;
	setEditStyle(hVarEdit, g_config->maxlenVarinfo);

	VarTree::init();

	// ポップアップメニューの追加
	hPopupOfVar = CreatePopupMenu();
		AppendMenu( hPopupOfVar, MF_STRING, IDM_VAR_LOGGING, "ログ(&L)");
		AppendMenu( hPopupOfVar, MF_STRING, IDM_VAR_UPDATE,  "更新(&U)" );
		AppendMenu( hPopupOfVar, MF_SEPARATOR, 0, 0 );
		AppendMenu( hPopupOfVar, MF_STRING, IDM_VAR_STEPOUT, "脱出(&O)" );
}

//------------------------------------------------
// 変数タブ::変数情報の更新
//------------------------------------------------
void TabVarsUpdate()
{
	HTREEITEM const hItem = TreeView_GetSelection(hVarTree);
	if ( hItem ) {
		string const varinfoText = VarTree::getItemVarText(hItem);
		if ( !varinfoText.empty() ) {
			Edit_UpdateText(hVarEdit, varinfoText.c_str());
		}
	}
}

//------------------------------------------------
// ログのチェックボックス
//------------------------------------------------
bool updatesLogAutomatically()
{
	return IsDlgButtonChecked( hLogPage, IDC_CHK_LOG_UPDATE ) != FALSE;
}

bool logsCalling()
{
	return IsDlgButtonChecked( hLogChkCalog, IDC_CHK_LOG_CALOG ) != FALSE;
}

//------------------------------------------------
// ログボックス
//------------------------------------------------
namespace LogBox {
	static HWND hwnd_;
	static string stock_;

	void init(HWND hwnd) {
		hwnd_ = hwnd;
		setEditStyle(hwnd, g_config->logMaxlen);
	}
	void clearImpl() {
		stock_.clear();
		Edit_SetSel(hwnd_, 0, -1);
		Edit_ReplaceSel(hwnd_, "");
	}
	void clear() {
		if ( !g_config->warnsBeforeClearingLog
			|| MessageBox(hDlgWnd, "ログをすべて消去しますか？", "knowbug", MB_OKCANCEL) == IDOK ) {
			clearImpl();
		}
	}
	void commit(char const* textAdd) {
		int caret[2];
		SendMessage(hwnd_, EM_GETSEL,
			(WPARAM)(&caret[0]),
			(LPARAM)(&caret[1])
		);

		int const size = Edit_GetTextLength(hwnd_);
		Edit_SetSel(hwnd_, size, size);  // 最後尾にキャレットを置く
		Edit_ReplaceSel(hwnd_, textAdd); // 文字列を追加する
		Edit_ScrollCaret(hwnd_);         // 画面を必要なだけスクロール

		// 選択状態を元に戻す
		Edit_SetSel(hwnd_, caret[0], caret[1]);
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
		// ログメッセージを取り出す
		int const size = Edit_GetTextLength(hwnd_);
		std::vector<char> buf(size + 2);
		GetWindowText(hwnd_, buf.data(), size + 1);

		// 保存
		HANDLE const hFile =
			CreateFile(filepath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if ( hFile != INVALID_HANDLE_VALUE ) {
			DWORD writesize;
			WriteFile(hFile, buf.data(), size, &writesize, nullptr);
			CloseHandle(hFile);
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
static void SrcSyncImpl( HWND hEdit, char const* p )
{
	Edit_UpdateText(hEdit, p);
}

static void SrcSync(char const* filepath, int line_num, bool bUpdateEdit, bool bUpdateBox)
{
	if ( !filepath || line_num < 0 ) return;

	if ( auto const p = ReadFromSourceFile(filepath) ) {
		assert(line_num >= 1);	// 行番号 line_num は 1-based
		size_t const iLine = static_cast<size_t>(line_num - 1);
		auto&& ran = p->lineRange(iLine);

		if ( bUpdateEdit ) {
			if ( stt_viewingFilepath != filepath ) {
				SrcSyncImpl(hSrcEdit, p->get().c_str());
				stt_viewingFilepath = filepath;
			}
			Edit_SetSel(hSrcEdit, ran.first, ran.second); // 該当行を選択
			Edit_Scroll(hSrcEdit, iLine, 0);
		}
		if ( bUpdateBox ) {
			SrcSyncImpl(hSrcBox, p->line(iLine).c_str());
		}
	} else {
		auto const&& text = DebugInfo::formatCurInfString(filepath, line_num);
		if ( bUpdateEdit ) SrcSyncImpl(hSrcEdit, text.c_str());
		if ( bUpdateBox ) SrcSyncImpl(hSrcBox, text.c_str());
	}
}

//------------------------------------------------
// ソースタブの更新
//------------------------------------------------
static void TabSrcUpdate()
{
	SrcSync(g_dbginfo->debug->fname, g_dbginfo->debug->line, true, false);
}

//------------------------------------------------
// 全般タブ::プロシージャ
//------------------------------------------------
LRESULT CALLBACK TabGeneralProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch ( msg ) {

		case WM_INITDIALOG:
			hGenList = GetDlgItem( hDlg, IDC_LV_GENERAL );
			hSrcBox  = GetDlgItem( hDlg, IDC_SRC_BOX );
			TabGeneralInit();
			TabGeneralUpdate();
			return TRUE;

		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
				case IDC_BTN_UPDATE:
					TabGeneralUpdate();
					break;
			}
			return FALSE;
	}
	return FALSE;
}

//------------------------------------------------
// 変数タブ::プロシージャ
//------------------------------------------------
LRESULT CALLBACK TabVarsProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch ( msg ) {
		case WM_INITDIALOG:
			TabVarInit( hDlg );
			return TRUE;

		case WM_CONTEXTMENU: {
			// ツリー上で逆クリック
			if ( (HWND)wp == hVarTree ) {
				TV_HITTESTINFO tvHitTestInfo;
				tvHitTestInfo.pt = { LOWORD(lp), HIWORD(lp) };
				ScreenToClient( hVarTree, &tvHitTestInfo.pt );
				auto const hItem = TreeView_HitTest( hVarTree, &tvHitTestInfo );
				if ( !hItem )  break;

				if ( tvHitTestInfo.flags & TVHT_ONITEMLABEL ) {		// 文字列アイテムの場合
					auto const nodeString = TreeView_GetItemString(hVarTree, hItem);
					{
						auto const menuText = strf("「%s」をログ(&L)", nodeString.c_str());
						MENUITEMINFO menuInfo;
							menuInfo.cbSize = sizeof(menuInfo);
							menuInfo.fMask  = MIIM_STRING;
							menuInfo.dwTypeData = const_cast<LPSTR>( menuText.c_str() );
						SetMenuItemInfo( hPopupOfVar, IDM_VAR_LOGGING, FALSE, &menuInfo );
					}

					// 「脱出」は呼び出しノードに対してのみ有効
					EnableMenuItem( hPopupOfVar, IDM_VAR_STEPOUT,
						(VarTree::InvokeNode::isTypeOf(nodeString.c_str()) ? MFS_ENABLED : MFS_DISABLED));

					// ポップアップメニューを表示する
					int const idSelected = TrackPopupMenuEx(
						hPopupOfVar, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
						(int)LOWORD(lp), (int)HIWORD(lp), hDlgWnd, nullptr
					);

					switch ( idSelected ) {
						case IDM_VAR_LOGGING: {
							string const&& varinfoText = VarTree::getItemVarText(hItem);
							Knowbug::logmes( varinfoText.c_str() );
							return TRUE;
						}
						case IDM_VAR_UPDATE:
							TabVarsUpdate();
							break;
						case IDM_VAR_STEPOUT: {
							assert(VarTree::InvokeNode::isTypeOf(nodeString.c_str()));
							auto const idx =  VarTree::TreeView_MyLParam<VarTree::InvokeNode>(hVarTree, hItem);
							if ( auto const pCallInfo = WrapCall::getCallInfoAt(idx) ) {
								// 対象が呼び出された階層まで進む
								Knowbug::runStepReturn(pCallInfo->sublev);
							}
							break;
						}
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
					// 選択項目が変化した
					case TVN_SELCHANGED:
					case NM_DBLCLK:
					case NM_RETURN:
						TabVarsUpdate();
						break;

					// カスタムドロー
					case NM_CUSTOMDRAW: {
						if ( !g_config->bCustomDraw ) break;
						LRESULT const res = VarTree::customDraw(reinterpret_cast<LPNMTVCUSTOMDRAW>(nmhdr));
						SetWindowLong( hDlg, DWL_MSGRESULT, res );
						return TRUE;
					}
				}
			}
			break;
		}
	}
	return FALSE;
}

//------------------------------------------------
// ログタブ::プロシージャ
//------------------------------------------------
LRESULT CALLBACK TabLogProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch ( msg ) {
		case WM_INITDIALOG:
			hLogPage = hDlg;
			hLogChkUpdate = GetDlgItem( hDlg, IDC_CHK_LOG_UPDATE );
			hLogChkCalog  = GetDlgItem( hDlg, IDC_CHK_LOG_CALOG );

			CheckDlgButton( hLogPage, IDC_CHK_LOG_UPDATE, BST_CHECKED );
#ifndef with_WrapCall
			EnableWindow( hLogChkCalog, false );
#endif
			LogBox::init(GetDlgItem( hDlg, IDC_LOG ));
			return TRUE;

		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
				case IDC_CHK_LOG_UPDATE:
					// チェックが付けられたとき
					if ( updatesLogAutomatically() ) { LogBox::update(); }
					break;
				case IDC_BTN_LOG_UPDATE: LogBox::update(); break;
				case IDC_BTN_LOG_SAVE:  LogBox::save(); break;
				case IDC_BTN_LOG_CLEAR: LogBox::clear(); break;
			}
			return FALSE;
	}
	return FALSE;
}

//------------------------------------------------
// ソースタブ::プロシージャ
//------------------------------------------------
LRESULT CALLBACK TabSrcProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch ( msg ) {
		case WM_INITDIALOG:
			hSrcPage = hDlg;
			hSrcEdit = GetDlgItem( hDlg, IDC_SRC );
			setEditStyle(hSrcEdit, g_config->logMaxlen); // ログの最大長を流用
			return TRUE;

		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
				case IDC_BTN_SRC_UPDATE:
					TabSrcUpdate();
					break;
			}
	}
	return FALSE;
}

//------------------------------------------------
// 親ダイアログのコールバック関数
//------------------------------------------------
LRESULT CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch ( msg ) {
		case WM_CREATE: {
			hPopup = CreatePopupMenu();
				AppendMenu(hPopup, (g_config->bTopMost ? MFS_CHECKED : MFS_UNCHECKED), IDM_TOPMOST, "常に最前面に表示する(&T)");
				AppendMenu(hPopup, MFS_DEFAULT, IDM_OPEN_INI, "設定ファイル(&I)");

			// ダイアログオブジェクトを生成
			auto const hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
			hTabCtrl = GenerateObj( hDlg, WC_TABCONTROL, "", DIALOG_X0, DIALOG_Y0, DIALOG_X1, DIALOG_Y2, IDU_TAB, hFont );
			hSttCtrl = GenerateObj( hDlg, "static",    "",  DIALOG_X0 + 210, DIALOG_Y1 + 12, DIALOG_X1 - 210, 48, 0, hFont );
			hBtn1    = GenerateObj( hDlg, "button", "実行", DIALOG_X0 +   8, DIALOG_Y1 + 12, 40, 24, ID_BTN1, hFont );
			hBtn2    = GenerateObj( hDlg, "button", "次行", DIALOG_X0 +  48, DIALOG_Y1 + 12, 40, 24, ID_BTN2, hFont );	
			hBtn3    = GenerateObj( hDlg, "button", "停止", DIALOG_X0 +  88, DIALOG_Y1 + 12, 40, 24, ID_BTN3, hFont );
			hBtn4    = GenerateObj( hDlg, "button", "次飛", DIALOG_X0 + 128, DIALOG_Y1 + 12, 40, 24, ID_BTN4, hFont );
			hBtn5    = GenerateObj( hDlg, "button", "脱出", DIALOG_X0 + 168, DIALOG_Y1 + 12, 40, 24, ID_BTN5, hFont );

			//generate tab dialogs
			{
				struct tabinfo {
					char const* label;
					char const* name;
					DLGPROC proc;
				};
				static std::array<tabinfo, TABDLGMAX> const stc_tabinfo = { {
					{ "全般", "T_GENERAL", (DLGPROC)TabGeneralProc },
					{ "変数", "T_VAR", (DLGPROC)TabVarsProc },
					{ "ログ", "T_LOG", (DLGPROC)TabLogProc },
					{ "スクリプト", "T_SRC", (DLGPROC)TabSrcProc }
				} };

				TCITEM tc;
				tc.mask = TCIF_TEXT;
				for ( int i = 0; i < TABDLGMAX; ++i ) {
					tc.pszText = const_cast<char*>( stc_tabinfo[i].label );
					TabCtrl_InsertItem(hTabCtrl, i, &tc);
					hTabSheet[i] = CreateDialog(Knowbug::getInstance(), stc_tabinfo[i].name, hDlg, stc_tabinfo[i].proc);
				}
			}

			RECT rt;
			SetRect( &rt, 8, DIALOG_Y2 + 4, DIALOG_X1 + 8, DIALOG_Y1 + 4 );

			//put child dialogs onto tab sheets
			for ( int i = 0; i < TABDLGMAX; ++ i ) {
				MoveWindow( hTabSheet[i],
					rt.left, rt.top,
					(rt.right  - rt.left), (rt.bottom - rt.top),
					FALSE
				);
			}

			//open initial tab
			int const initialTab = g_config->initialTab;
			TabCtrl_SetCurSel(hTabCtrl, (0 <= initialTab && initialTab < TABDLGMAX ? initialTab : 0));
			ShowWindow(hTabSheet[TabCtrl_GetCurSel(hTabCtrl)], SW_SHOW);
			return TRUE;
		}
		case WM_NOTIFY: {
			auto const nm = reinterpret_cast<NMHDR*>(lp);
			if ( nm->code == TCN_SELCHANGE ) { // タブコントロールのシート切り替え通知
				int const cur = TabCtrl_GetCurSel(hTabCtrl);
				for ( int i = 0; i < TABDLGMAX; ++i ) {
					ShowWindow(hTabSheet[i], (i == cur ? SW_SHOW : SW_HIDE));
				}
				update();
			}
			break;
		}
		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
				case ID_BTN1: Knowbug::run();         break;
				case ID_BTN2: Knowbug::runStepIn();   break;
				case ID_BTN3: Knowbug::runStop();     break;
				case ID_BTN4: Knowbug::runStepOver(); break;
				case ID_BTN5: Knowbug::runStepOut();  break;
			}
			if ( LOWORD(wp) != ID_BTN3 ) SetWindowText( Dialog::hSttCtrl, "" );
			break;

		case WM_CONTEXTMENU:		// ポップアップメニュー表示
		{
			POINT pt;
			GetCursorPos( &pt );	// カーソル位置 (スクリーン座標)

			// ポップアップメニューを表示する
			int const idSelected = TrackPopupMenuEx(
				hPopup, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, hDlg, nullptr
			);
			switch ( idSelected ) {
				case IDM_TOPMOST: {
					g_config->bTopMost = !g_config->bTopMost;

					MENUITEMINFO menuInfo;
						menuInfo.cbSize = sizeof(menuInfo);
						menuInfo.fMask  = MIIM_STATE;
						menuInfo.fState = ( g_config->bTopMost ? MFS_CHECKED : MFS_UNCHECKED );
					SetMenuItemInfo( hPopup, IDM_TOPMOST, FALSE, &menuInfo );

					SetWindowPos(	// 最前面
						hDlgWnd, (g_config->bTopMost ? HWND_TOPMOST : HWND_NOTOPMOST),
						0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)
					);
					break;
				}
				case IDM_OPEN_INI: {
					std::ofstream of { g_config->selfPath(), std::ios::app }; //create empty file if not exist
					ShellExecute(nullptr, "open", g_config->selfPath().c_str(), nullptr, "", SW_SHOWDEFAULT);	
					break;
				}
				default: break;
			}
			return TRUE;
		}
		case WM_CLOSE:
			return FALSE;

		case WM_DESTROY:
			DestroyMenu( hPopup );      hPopup      = nullptr;
			DestroyMenu( hPopupOfVar ); hPopupOfVar = nullptr;
			PostQuitMessage(0);
			break;
		}
		return DefWindowProc(hDlg, msg, wp, lp) ;
}

//------------------------------------------------
// メインダイアログを生成する
//------------------------------------------------
HWND Dialog::createMain()
{
	int const dispx = GetSystemMetrics( SM_CXSCREEN );
	int const dispy = GetSystemMetrics( SM_CYSCREEN );

	WNDCLASS wndclass;
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = DlgProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = Knowbug::getInstance();
	wndclass.hIcon         = nullptr;
	wndclass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndclass.lpszMenuName  = nullptr;
	wndclass.lpszClassName = myClass;
	RegisterClass( &wndclass );

	hDlgWnd = CreateWindow(
		myClass,
		"Debug Window",
		(WS_CAPTION | WS_OVERLAPPED | WS_BORDER | WS_VISIBLE),
		dispx - WND_SX, 0,
		WND_SX, WND_SY,
		nullptr,
		nullptr,
		Knowbug::getInstance(),
		nullptr
	);
	if ( !hDlgWnd ) {
		MessageBox( nullptr, "Debug window initalizing failed.", "Error", 0 );
		abort();
	}

	SetWindowPos( //top most
		hDlgWnd, (g_config->bTopMost ? HWND_TOPMOST : HWND_NOTOPMOST),
		0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)
	);
	ShowWindow( hDlgWnd, SW_SHOW );
	UpdateWindow( hDlgWnd );
	return hDlgWnd;
}

void Dialog::destroyMain()
{
	if ( !g_config->logPath.empty() ) { //auto save
		LogBox::save(g_config->logPath.c_str());
	}

	if ( hDlgWnd != nullptr ) {
		DestroyWindow( hDlgWnd );
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

	int const idxTab = TabCtrl_GetCurSel( hTabCtrl );
	switch( idxTab ) {
		case 0: TabGeneralUpdate(); break;
		case 1: TabVarsUpdate(); break;
		case 2: break;
		case 3: TabSrcUpdate(); break;
	}
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

//##############################################################################
//                下請け処理
//##############################################################################
//------------------------------------------------
// EditControl のタブ文字幅を変更する
//------------------------------------------------
void Edit_SetTabLength( HWND hEdit, const int tabwidth )
{
	HDC const hdc = GetDC(hEdit);
	{
		TEXTMETRIC tm;
		if ( GetTextMetrics(hdc, &tm) ) {
			int const tabstops = tm.tmAveCharWidth / 4 * tabwidth * 2;
			SendMessage( hEdit, EM_SETTABSTOPS, 1, (LPARAM)(&tabstops) );
		}
	}
	ReleaseDC( hEdit, hdc );
}

//------------------------------------------------
// EditControl の文字列の置き換え
//------------------------------------------------
void Edit_UpdateText(HWND hwnd, char const* s)
{
	int const vscrollBak = Edit_GetFirstVisibleLine(hwnd);
	SetWindowText(hwnd, s);
	Edit_Scroll(hwnd, vscrollBak, 0);
}

//------------------------------------------------
// ツリービューの項目ラベルを取得する
//------------------------------------------------
string TreeView_GetItemString( HWND hwndTree, HTREEITEM hItem )
{
	char stmp[256];

	TVITEM ti;
	ti.hItem      = hItem;
	ti.mask       = TVIF_TEXT;
	ti.pszText    = stmp;
	ti.cchTextMax = sizeof(stmp) - 1;

	return (TreeView_GetItem( hwndTree, &ti ) ? stmp : "");
}

//------------------------------------------------
// ツリービューのノードに関連する lparam 値を取得する
//------------------------------------------------
LPARAM TreeView_GetItemLParam( HWND hwndTree, HTREEITEM hItem )
{
	TVITEM ti;
	ti.hItem = hItem;
	ti.mask  = TVIF_PARAM;

	TreeView_GetItem( hwndTree, &ti );
	return ti.lParam;
}

//------------------------------------------------
// ツリービューのフォーカスを回避する
// 
// @ 対象のノードが選択状態なら、その兄ノードか親ノードを選択する。
//------------------------------------------------
void TreeView_EscapeFocus( HWND hwndTree, HTREEITEM hItem )
{
	if ( TreeView_GetSelection(hwndTree) == hItem ) {
		HTREEITEM hUpper = TreeView_GetPrevSibling( hwndTree, hItem );
		if ( !hUpper ) hUpper = TreeView_GetParent(hwndTree, hItem);

		TreeView_SelectItem( hwndTree, hUpper );
	}
}

//------------------------------------------------
// 末子ノードを取得する (failure: nullptr)
//------------------------------------------------
HTREEITEM TreeView_GetChildLast(HWND hwndTree, HTREEITEM hItem)
{
	HTREEITEM hLast = TreeView_GetChild(hwndTree, hItem);
	if ( !hLast ) return nullptr;	// error

	for ( HTREEITEM hNext = hLast
		; hNext != nullptr
		; hNext = TreeView_GetNextSibling(hwndTree, hLast)
		) {
		hLast = hNext;
	}
	return hLast;
}
