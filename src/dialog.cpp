
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
static HWND hLogEdit;
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
static void TabGeneralInit( void )
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

static void TabGeneralReset()
{
	ListView_DeleteAllItems( hGenList );

	int tgmax = 0;
	int chk;
	char name[256];
	char val[512];

	// HSP側に問い合わせ
	std::unique_ptr<char, void(*)(char*)> p(
		g_dbginfo->debug->get_value( DEBUGINFO_GENERAL ),
		g_dbginfo->debug->dbg_close
	);
	strsp_ini();
	for (;;) {
		chk = strsp_get( p.get(), name, 0, sizeof(name) - 1 );
		if ( chk == 0 ) break;
		
		chk = strsp_get( p.get(), val, 0, sizeof(val) - 1 );
		if ( chk == 0 ) break;
		
		TabGeneral_AddItem( name, val, tgmax ); ++tgmax;
	}
	
	// 拡張内容の追加
	if ( exinfo->actscr ) {
		auto const pBmscr = reinterpret_cast<BMSCR*>(exinfo->HspFunc_getbmscr(*exinfo->actscr));
		if ( pBmscr ) {
			// color
			{
				COLORREF const cref = pBmscr->color;
				sprintf_s(val, "(%d, %d, %d)",
					GetRValue(cref), GetGValue(cref), GetBValue(cref)
				);
			}
			TabGeneral_AddItem("color", val, tgmax); ++tgmax;

			// pos
			sprintf_s(val, "(%d, %d)", pBmscr->cx, pBmscr->cy);
			TabGeneral_AddItem("pos", val, tgmax); ++tgmax;
		}
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
	setEditStyle( hVarEdit );
	
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
	HTREEITEM const hItem = TreeView_GetSelection( hVarTree );
	if ( !hItem ) return;
	
	string const varinfoText = VarTree::getItemVarText(hItem);
	if ( !varinfoText.empty() ) {
		Edit_UpdateText(hVarEdit, varinfoText.c_str());
	}
}

//------------------------------------------------
// ログのチェックボックス
//------------------------------------------------
bool isLogAutomaticallyUpdated()
{
	return IsDlgButtonChecked( hLogPage, IDC_CHK_LOG_UPDATE ) != FALSE;
}

bool isLogCallings()
{
	return IsDlgButtonChecked( hLogChkCalog, IDC_CHK_LOG_CALOG ) != FALSE;
}

//------------------------------------------------
// ログメッセージを初期化する
//------------------------------------------------
static string stt_logmsg;

void logClear()
{
	stt_logmsg.clear();
	
	Edit_SetSel( hLogEdit, 0, -1 );
	Edit_ReplaceSel( hLogEdit, "" );
}

//------------------------------------------------
// ログメッセージを追加・更新する
//------------------------------------------------
void logUpdate( char const* textAdd )
{
	int caret[2];
	SendMessage( hLogEdit, EM_GETSEL,
		(WPARAM)( &caret[0] ),
		(LPARAM)( &caret[1] )
	);
	
	int const size = Edit_GetTextLength( hLogEdit );
	Edit_SetSel( hLogEdit, size, size );  // 最後尾にキャレットを置く
	Edit_ReplaceSel( hLogEdit, textAdd ); // 文字列を追加する
	Edit_ScrollCaret( hLogEdit );         // 画面を必要なだけスクロール
	
	// 選択状態を元に戻す
	Edit_SetSel( hLogEdit, caret[0], caret[1] );
}

//------------------------------------------------
// ログメッセージを更新する (commit)
//------------------------------------------------
void TabLogCommit()
{
	if ( stt_logmsg.empty() ) return;
	
	logUpdate( stt_logmsg.c_str() );
	stt_logmsg.clear();
}

//------------------------------------------------
// ログメッセージに追加する
//------------------------------------------------
void logAdd( char const* str )
{
	// 自動更新
	if ( isLogAutomaticallyUpdated() ) {
		logUpdate( str );
		
	} else {
		stt_logmsg.append( str );
	}
}

void logAddCrlf()
{
	logAdd( "\r\n" );
}

// 現在位置を更新して、現在位置をログに追加する。
void logAddCurInf()
{
	g_dbginfo->debug->dbg_curinf();
	logAdd(("CurInf:" + g_dbginfo->getCurInfString()).c_str());
	logAddCrlf();
}

//------------------------------------------------
// ログメッセージを保存する
//------------------------------------------------
void logSave()
{
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
		logSave(fullname);
	}
}

void logSave( char const* filepath )
{
	// ログメッセージを取り出す
	int const size = Edit_GetTextLength( hLogEdit );
	std::vector<char> buf(size + 2);
	GetWindowText( hLogEdit, buf.data(), size + 1 );
	
	// 保存
	HANDLE const hFile =
		CreateFile( filepath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( hFile != INVALID_HANDLE_VALUE ) {
		DWORD writesize;
		WriteFile( hFile, buf.data(), size, &writesize, nullptr );
		CloseHandle( hFile );
	}
}

//------------------------------------------------
// ソースファイルを開く
//
// @ エディタ上で編集中の場合、ファイルの内容が実際と異なることがある。行番号のアウトレンジに注意。
//------------------------------------------------
/*
// 行ごとに分割されたテキスト
first: スクリプト全体、ただし各行の字下げ空白は除去されている。
second: 行番号(0ベース)の添字に対して、first におけるその行の先頭へのオフセット値。
	最初の要素は 0、最後の要素は first の長さ。
*/
using script_t = std::pair<string const, std::vector<size_t>>;

// 現在ソースタブで表示されているファイルのパス
static string stt_viewingFilepath;

// 読み込み処理 (failure: nullptr)
static script_t const* ReadFromSourceFile( char const* _filepath )
{
	static std::map<string const, script_t> stt_src_cache;
	
	string const filepath = _filepath;
	
	// キャッシュから検索
	{
		auto const iter = stt_src_cache.find(filepath);
		if ( iter != stt_src_cache.end() ) return &iter->second;
	}

	// ファイルから読み込む
	string code;
	std::vector<size_t> idxlist;
	{
		std::ifstream ifs { filepath };
		if ( !ifs.is_open() ) { //search "common" folder
			char path[MAX_PATH];
			if ( SearchPath( g_config->commonPath.c_str(), _filepath, nullptr, sizeof(path), path, nullptr ) == 0 ) {
				return nullptr;
			}
			
			ifs.open( path );
			if ( !ifs.is_open() ) return nullptr;
		}
		
		char linebuf[0x400];
		size_t idx = 0;
		idxlist.push_back( 0 );
		do {
			ifs.getline( linebuf, sizeof(linebuf) );
			int cntIndents = 0; {
				for( int& i = cntIndents; linebuf[i] == '\t' || linebuf[i] == ' '; ++ i );
			}
			char const* const p = &linebuf[cntIndents];
			size_t const len = std::strlen(p);
			code.append( p, p + len ).append("\r\n");
			idx += len + 2;
			idxlist.push_back( idx );
		} while ( ifs.good() );
	}

	auto const res =
		stt_src_cache.emplace(std::move(filepath), make_pair(std::move(code), std::move(idxlist)));
	return &res.first->second;
}

// ソースタブを同期する
static void SrcSyncImpl( HWND hEdit, char const* p )
{
	Edit_UpdateText(hEdit, p);
}

static void SrcSync( char const* filepath, int line_num, bool bUpdateEdit, bool bUpdateBox )
{
	if ( !filepath || line_num < 0 ) return;

	if ( auto const p = ReadFromSourceFile( filepath ) ) {
		assert(line_num >= 1);	// 行番号 line_num は 1-based
		size_t const iLine = static_cast<size_t>(line_num) - 1;

		size_t idxlist[2];
		if ( iLine + 1 < p->second.size() ) {
			idxlist[0] = p->second[iLine]; idxlist[1] = p->second[iLine + 1];
		} else {
			idxlist[0] = idxlist[1] = 0;
		}
		if ( bUpdateEdit ) {
			if ( stt_viewingFilepath.empty() || stt_viewingFilepath != filepath ) {
				SrcSyncImpl(hSrcEdit, p->first.c_str());
				stt_viewingFilepath = filepath;
			}
			Edit_SetSel( hSrcEdit, idxlist[0], idxlist[1] );	// 該当行を選択
			Edit_Scroll( hSrcEdit, iLine, 0 );
		}
		if ( bUpdateBox ) {
			auto const text = p->first.substr( idxlist[0], idxlist[1] - idxlist[0] );
			SrcSyncImpl( hSrcBox, text.c_str() );
		}
	} else {
		auto const text = strf("(#%d \"%s\")", line_num, filepath);
		if ( bUpdateEdit ) SrcSyncImpl( hSrcEdit, text.c_str() );
		if ( bUpdateBox  ) SrcSyncImpl( hSrcBox,  text.c_str() );
	}
}

//------------------------------------------------
// ソースタブの更新
//------------------------------------------------
static void TabSrcUpdate()
{
	SrcSync( g_dbginfo->debug->fname, g_dbginfo->debug->line, true, false );
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
			TabGeneralReset();
			return TRUE;
			
		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
				case IDC_BTN_UPDATE:
					TabGeneralReset();
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
			
		case WM_CONTEXTMENU:
		{
			// ツリー上で逆クリック
			if ( wp == (WPARAM)hVarTree ) {
				TV_HITTESTINFO tvHitTestInfo;
					tvHitTestInfo.pt.x = LOWORD(lp);
					tvHitTestInfo.pt.y = HIWORD(lp);
				ScreenToClient( hVarTree, &tvHitTestInfo.pt );
				auto const hItem = TreeView_HitTest( hVarTree, &tvHitTestInfo );	// 対象を確定
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
						(VarTree::isCallNode(nodeString.c_str()) ? MFS_ENABLED : MFS_DISABLED));
					
					// ポップアップメニューを表示する
					int const idSelected = TrackPopupMenuEx(
						hPopupOfVar, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
						(int)LOWORD(lp), (int)HIWORD(lp), hDlgWnd, nullptr
					);
					
					switch ( idSelected ) {
						case IDM_VAR_LOGGING:
						{
							string const varinfoText = VarTree::getItemVarText(hItem);
							Knowbug::logmes( varinfoText.c_str() );		// logmes 送信
							return TRUE;
						}
						case IDM_VAR_UPDATE:
							TabVarsUpdate();
							break;
						case IDM_VAR_STEPOUT:
						{
							assert(VarTree::isCallNode(nodeString.c_str()));
							auto const idx = static_cast<int>( TreeView_GetItemLParam(hVarTree, hItem) );
							assert(idx >= 0);
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
		case WM_NOTIFY:
		{
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
					case NM_CUSTOMDRAW:
					{
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
			hLogEdit = GetDlgItem( hDlg, IDC_LOG );
			hLogChkUpdate = GetDlgItem( hDlg, IDC_CHK_LOG_UPDATE );
			hLogChkCalog  = GetDlgItem( hDlg, IDC_CHK_LOG_CALOG );
			
			CheckDlgButton( hLogPage, IDC_CHK_LOG_UPDATE, BST_CHECKED );
#ifdef with_WrapCall
		//	CheckDlgButton( hLogPage, IDC_CHK_LOG_CALOG,  BST_CHECKED );
#else
			EnableWindow( hLogChkCalog, false );
#endif
			
			setEditStyle( hLogEdit );
			SendMessage( hLogEdit, EM_SETLIMITTEXT, (WPARAM) g_config->logMaxlen, 0 );

			stt_logmsg.reserve( 0x400 + 1 );
			return TRUE;
			
		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
				
				case IDC_CHK_LOG_UPDATE:
					// チェックが付けられたとき
					if ( IsDlgButtonChecked(hLogPage, IDC_CHK_LOG_UPDATE) ) { TabLogCommit(); }
					break;
					
				case IDC_BTN_LOG_UPDATE:
					TabLogCommit();
					break;
					
				case IDC_BTN_LOG_SAVE:
					logSave();
					break;
					
				case IDC_BTN_LOG_CLEAR:
					logClear();
					break;
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
			
			setEditStyle( hSrcEdit );
			SendMessage( hSrcEdit, EM_SETLIMITTEXT, (WPARAM) g_config->logMaxlen, 0 );	// ログの最大長を流用
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

	// 初期化
	case WM_CREATE:
	{
		// ポップアップメニューを生成
		hPopup = CreatePopupMenu();
			AppendMenu( hPopup, (g_config->bTopMost ? MFS_CHECKED : MFS_UNCHECKED), IDM_TOPMOST, "常に最前面に表示する(&T)" );
		
		// ダイアログオブジェクトを生成
		auto const hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
		hTabCtrl = GenerateObj( hDlg, WC_TABCONTROL, "", DIALOG_X0, DIALOG_Y0, DIALOG_X1, DIALOG_Y2, IDU_TAB, hFont );
		hSttCtrl = GenerateObj( hDlg, "static",    "",  DIALOG_X0 + 210, DIALOG_Y1 + 12, DIALOG_X1 - 210, 48, 0, hFont );
		hBtn1    = GenerateObj( hDlg, "button", "実行", DIALOG_X0 +   8, DIALOG_Y1 + 12, 40, 24, ID_BTN1, hFont );
		hBtn2    = GenerateObj( hDlg, "button", "次行", DIALOG_X0 +  48, DIALOG_Y1 + 12, 40, 24, ID_BTN2, hFont );	
		hBtn3    = GenerateObj( hDlg, "button", "停止", DIALOG_X0 +  88, DIALOG_Y1 + 12, 40, 24, ID_BTN3, hFont );
		hBtn4    = GenerateObj( hDlg, "button", "次飛", DIALOG_X0 + 128, DIALOG_Y1 + 12, 40, 24, ID_BTN4, hFont );
		hBtn5    = GenerateObj( hDlg, "button", "脱出", DIALOG_X0 + 168, DIALOG_Y1 + 12, 40, 24, ID_BTN5, hFont );
		
		// 全般タブ、変数タブ、ログタブ、スクリプトタブを追加
		{
			struct tabinfo {
				char const* const label;
				char const* const name;
				DLGPROC proc;
			};
			static std::array<tabinfo, TABDLGMAX> stc_tabinfo = { {
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

		// 生成した子ダイアログをタブシートの上に貼り付ける
		for ( int i = 0; i < TABDLGMAX; ++ i ) {
			MoveWindow(
				hTabSheet[i],
				rt.left, rt.top,
				(rt.right  - rt.left), (rt.bottom - rt.top),
				FALSE
			);
		}

		// デフォルトで左端のタブを表示
		ShowWindow( hTabSheet[0], SW_SHOW );
		return TRUE;
	}
	case WM_NOTIFY:
	{
		auto const nm = reinterpret_cast<NMHDR*>(lp);		// タブコントロールのシート切り替え通知
		int const cur = TabCtrl_GetCurSel(hTabCtrl);
		for ( int i = 0; i < TABDLGMAX; ++ i ) {
			ShowWindow( hTabSheet[i], (i == cur ? SW_SHOW : SW_HIDE) );
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
			case IDM_TOPMOST:
			{
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
		case 0:
			TabGeneralReset();
			break;
		case 1:
			TabVarsUpdate();
			break;
		case 3:
			TabSrcUpdate();
			break;
	}
}

//------------------------------------------------
// エディットコントロールの標準スタイル
// 
// @ 設定に依存
//------------------------------------------------
void setEditStyle( HWND hEdit )
{
	Edit_SetTabLength( hEdit, g_config->tabwidth );
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
