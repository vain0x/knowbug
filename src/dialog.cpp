
#pragma comment(lib, "comctl32.lib")

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "module/hspdll.h"
#include "module/supio.h"
#include "module/SortNote.h"

#include "dialog.h"
#include "vartree.h"
#include "config_mng.h"
#include "WrapCall.h"

#include <fstream>
#include <map>
#include <vector>

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
static HWND GenerateObj( HWND parent, char *name, char *ttl, int x, int y, int sx, int sy, int menu, HFONT font )
{
	HWND h = CreateWindow(
		name, ttl,
		(WS_CHILD | WS_VISIBLE),
		x, y, sx, sy,
		parent,
		(HMENU)menu,
		g_hInstance,
		NULL
	);

	if ( font != NULL ) SendMessage( h, WM_SETFONT, (WPARAM)font, TRUE );
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
	return;
}

//------------------------------------------------
// 全般タブの更新
//------------------------------------------------
static void SrcSync( const char* filepath, int line_num, bool bUpdateEdit, bool bUpdateBox );
static void TabGeneral_AddItem( const char *sItem, const char *sValue, int iItem );

static void TabGeneralReset( void )
{
	int chk, tgmax;
	char *p;
	char name[256];
	char val[512];

	ListView_DeleteAllItems( hGenList );
	tgmax = 0;

	p = g_debug->get_value( DEBUGINFO_GENERAL );		// HSP側に問い合わせ
	strsp_ini();
	for (;;) {
		chk = strsp_get( p, name, 0, 255 );
		if ( chk == 0 ) break;
		
		chk = strsp_get( p, val, 0, 511 );
		if ( chk == 0 ) break;
		
		TabGeneral_AddItem( name, val, tgmax );
		tgmax ++;
	}
	
	// 拡張内容の追加
	do if ( exinfo->actscr != NULL ) {
		BMSCR* pBmscr(
			reinterpret_cast<BMSCR*>( exinfo->HspFunc_getbmscr( *exinfo->actscr ) )
		);
		if ( pBmscr == NULL ) break;
		
		// color
		{
			COLORREF cref ( pBmscr->color );
			sprintf_s( val, "(%d, %d, %d)",
				GetRValue(cref), GetGValue(cref), GetBValue(cref)
			);
		}
		TabGeneral_AddItem( "color", val, tgmax );
		tgmax ++;
		
		// pos
		sprintf_s( val, "(%d, %d)", pBmscr->cx, pBmscr->cy );
		TabGeneral_AddItem( "pos", val, tgmax );
		tgmax ++;
	} while ( false );
	
	g_debug->dbg_close( p );

	SrcSync( g_debug->fname, g_debug->line, false, true );
	return;
}

static void TabGeneral_AddItem( const char *sItem, const char *sValue, int iItem )
{
	LV_ITEM item;
	
	item.mask     = LVIF_TEXT;
	item.iItem    = iItem;
	
	item.iSubItem = 0;
	item.pszText  = const_cast<char *>(sItem);
	ListView_InsertItem( hGenList, &item );
	
	item.iSubItem = 1;
	item.pszText  = const_cast<char *>(sValue);
	ListView_SetItem( hGenList, &item );
	return;
}

//------------------------------------------------
// 実行中の位置を更新する (line, file)
//------------------------------------------------
static void CurrnetUpdate( void )
{
	char tmp[512];
	char *fn;
	g_debug->dbg_curinf();
	fn = g_debug->fname;
	if ( fn == NULL ) fn = "???";
	sprintf_s( tmp, "%s\n( line : %d )", fn, g_debug->line );
	SetWindowText( hSttCtrl, tmp );
	
#ifdef with_khad
	if ( hKhad != NULL ) {
		SendMessage( hKhad, UWM_KHAD_CURPOS, g_debug->line, (LPARAM)g_debug->fname );
	}
#endif
	return;
}

//------------------------------------------------
// 変数タブのオプションを取得
//------------------------------------------------
static int GetTabVarsOption( void )
{
	return 0xF;
/*
	int opt( 0 );
	if ( IsDlgButtonChecked( hVarPage, IDC_MODULE ) ) opt |= 2;
	if ( IsDlgButtonChecked( hVarPage, IDC_ARRAY  ) ) opt |= 4;
	if ( IsDlgButtonChecked( hVarPage, IDC_DUMP   ) ) opt |= 8;
	return opt;
//*/
}

//------------------------------------------------
// 変数タブの初期化
//------------------------------------------------
void TabVarInit( HWND hDlg )
{
	hVarPage = hDlg;
	hVarTree = GetDlgItem( hDlg, IDC_VARTREE );
	hVarEdit = GetDlgItem( hDlg, IDC_VARINFO );
	setEditStyle( hVarEdit );
	
	VarTree::init( hVarTree );
	
	// ポップアップメニューの追加
	hPopupOfVar = CreatePopupMenu();
		AppendMenu( hPopupOfVar, MF_STRING, IDM_VAR_LOGGING, "ログ(&L)");	// 文は表示時に上書きされる
		AppendMenu( hPopupOfVar, MF_STRING, IDM_VAR_UPDATE,  "更新(&U" );
		AppendMenu( hPopupOfVar, MF_SEPARATOR, 0, 0 );
		AppendMenu( hPopupOfVar, MF_STRING, IDM_VAR_STEPOUT, "脱出(&O)" );
	return;
}

//------------------------------------------------
// 変数タブ::変数情報の更新
//------------------------------------------------
void TabVarsUpdate( void )
{
	HTREEITEM hItem = TreeView_GetSelection( hVarTree );
	if ( hItem == NULL ) return;
	
	CString&& varinfoText = VarTree::getItemVarText( hVarTree, hItem );
	if ( !varinfoText.empty() ) {
		SetWindowText( hVarEdit, varinfoText.c_str() );
	}
	return;
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
	Edit_ReplaceSel( hLogEdit, "" );		// 空っぽにする
	return;
}

//------------------------------------------------
// ログメッセージを追加・更新する
//------------------------------------------------
void logUpdate( const char* textAdd )
{
	int caret[2];
	SendMessage( hLogEdit, EM_GETSEL,
		(WPARAM)( &caret[0] ),
		(LPARAM)( &caret[1] )
	);
	
	int size = Edit_GetTextLength( hLogEdit );
	Edit_SetSel( hLogEdit, size, size );		// 最後尾にキャレットを置く
	Edit_ReplaceSel( hLogEdit, textAdd );		// 文字列を追加する
	Edit_ScrollCaret( hLogEdit );				// 画面を必要なだけスクロール
	
	// 選択状態を元に戻す
	Edit_SetSel( hLogEdit, caret[0], caret[1] );
	return;
}

//------------------------------------------------
// ログメッセージを更新する (commit)
//------------------------------------------------
void TabLogCommit()
{
	if ( stt_logmsg.empty() ) return;
	
	logUpdate( stt_logmsg.c_str() );
	stt_logmsg.clear();
	return;
}

//------------------------------------------------
// ログメッセージに追加する
//------------------------------------------------
void logAdd( const char *str )
{
	// 自動更新
	if ( IsDlgButtonChecked( hLogPage, IDC_CHK_LOG_UPDATE ) ) {
		logUpdate( str );
		
	} else {
		stt_logmsg.append( str );
	}
	return;
}

void logAddCrlf()
{
	logAdd( "\r\n" );
}

//------------------------------------------------
// ログメッセージを保存する
//------------------------------------------------
void logSave()
{
	char filename[MAX_PATH + 1] = "";
	char fullname[MAX_PATH + 1] = "hspdbg.log";
	OPENFILENAME ofn = { 0 };
		ofn.lStructSize    = sizeof(ofn);			// 構造体のサイズ
		ofn.hwndOwner      = hDlgWnd;				// コモンダイアログの親ウィンドウハンドル
		ofn.lpstrFilter    = "log text(*.txt;*.log)\0*.txt;*.log\0All files(*.*)\0*.*\0\0";	// ファイルの種類
		ofn.lpstrFile      = fullname;				// 選択されたファイル名(フルパス)を受け取る変数のアドレス
		ofn.lpstrFileTitle = filename;				// 選択されたファイル名を受け取る変数のアドレス
		ofn.nMaxFile       = sizeof(fullname);		// lpstrFileに指定した変数のサイズ
		ofn.nMaxFileTitle  = sizeof(filename);		// lpstrFileTitleに指定した変数のサイズ
		ofn.Flags          = OFN_OVERWRITEPROMPT;	// フラグ指定
		ofn.lpstrTitle     = "名前を付けて保存";	// コモンダイアログのキャプション
		ofn.lpstrDefExt    = "log";					// デフォルトのファイルの種類

	if ( !GetSaveFileName( &ofn ) ) return;
	
	logSave( fullname );
	return;
}

void logSave( const char* filepath )
{
	// ログメッセージを取り出す
	int size = Edit_GetTextLength( hLogEdit );
	char* buf = new char[size + 2];
	GetWindowText( hLogEdit, buf, size + 1 );
	
	// 保存
	HANDLE hFile = CreateFile( filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile != INVALID_HANDLE_VALUE ) {
		DWORD writesize;
		WriteFile( hFile, buf, size, &writesize, NULL );
		CloseHandle( hFile );
	}

	delete buf;
	return;
}

//------------------------------------------------
// ソースファイルを開く
//------------------------------------------------
#include "module/mod_chain.h"

typedef std::pair<const std::string, std::vector<size_t>> script_t;	// 行ごとに分割されたテキスト
static const char* stt_viewingFilepath = nullptr;	// 現在エディタにロードされているファイルのパス

// 読み込み処理
// @ 失敗 => nullptr
static const script_t* ReadFromSourceFile( const char* _filepath )
{
	static std::map<std::string, script_t> stt_src_cache;
	
	const string filepath = _filepath;
	
	// キャッシュから検索
	{
		auto iter = stt_src_cache.find(filepath);
		if ( iter != stt_src_cache.end() ) return &iter->second;
	}

	// ファイルから読み込む
	std::string code;
	std::vector<size_t> idxlist;
	{
		std::ifstream ifs( filepath );
		if ( !ifs.is_open() ) {			// 見つからなかった => common からも検索
			char path[MAX_PATH];
			if ( SearchPath( g_config->commonPath.c_str(), _filepath, NULL, sizeof(path), path, NULL ) == 0 ) {
				return nullptr;
			}
			
			ifs.open( path );
			if  ( !ifs.is_open() ) return nullptr;
		}
		
		char linebuf[320];
		size_t idx = 0;
		idxlist.push_back( 0 );
		do {
			ifs.getline( linebuf, sizeof(linebuf) );
			int cntIndents = 0; {
				for( int& i = cntIndents; linebuf[i] == '\t' || linebuf[i] == ' '; ++ i );
			}
			const char* const p = &linebuf[cntIndents];
			const size_t len = std::strlen(p);
			code.append( p, p + len ).append("\r\n");
			idx += len + 2;
			idxlist.push_back( idx );
		} while ( ifs.good() );
	}
	
	auto iter = stt_src_cache.insert(
			std::pair<std::string, script_t>( std::move(filepath), script_t( std::move(code), std::move(idxlist) ) )
		).first;
	stt_viewingFilepath = iter->first.c_str();
	return &iter->second;
}

// ソースタブを同期する
static void SrcSyncImpl( HWND hEdit, const char* p )
{
	Edit_SetSel( hEdit, 0, -1 );	// 全体を置き換える
	Edit_ReplaceSel( hEdit, p );
	return;
}

static void SrcSync( const char* filepath, int line_num, bool bUpdateEdit, bool bUpdateBox )
{
	if ( !filepath || line_num < 0 ) return;
	
	auto const* p = ReadFromSourceFile( filepath );
	
	if ( p ) {
		const size_t iLine = std::min( static_cast<size_t>(line_num), p->second.size() ) - 1;	// 行番号は 1-based
		const size_t idxlist[2] = { p->second[iLine], p->second[iLine + 1] };
		if ( bUpdateEdit ) {
			SrcSyncImpl( hSrcEdit, p->first.c_str() );
			Edit_SetSel( hSrcEdit, idxlist[0], idxlist[0] );
			Edit_Scroll( hSrcEdit, iLine, 0 );
		}
		if ( bUpdateBox ) {
			SrcSyncImpl( hSrcBox, p->first.substr( idxlist[0], (idxlist[1] - 2) - idxlist[0] ).c_str() );
		}
	} else {
		auto text = strf("(#%d of %s)", line_num, filepath);
		if ( bUpdateEdit ) SrcSyncImpl( hSrcEdit, text.c_str() );
		if ( bUpdateBox  ) SrcSyncImpl( hSrcBox,  text.c_str() );
	}
	return;
}

//------------------------------------------------
// ソースタブの更新
//------------------------------------------------
static void TabSrcUpdate()
{
	SrcSync( g_debug->fname, g_debug->line, true, false );
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
				const auto hItem = TreeView_HitTest( hVarTree, &tvHitTestInfo );	// 対象を確定
				if ( hItem == NULL )  break;
				
				if ( tvHitTestInfo.flags & TVHT_ONITEMLABEL ) {		// 文字列アイテムの場合
					const auto varname = TreeView_GetItemString(hVarTree, hItem);
					{
						const auto menuText = strf( "「%s」をログ(&L)", varname.c_str() );
						MENUITEMINFO menuInfo;
							menuInfo.cbSize = sizeof(menuInfo);
							menuInfo.fMask  = MIIM_STRING;
							menuInfo.dwTypeData = const_cast<LPSTR>( menuText.c_str() );
						SetMenuItemInfo( hPopupOfVar, IDM_VAR_LOGGING, FALSE, &menuInfo );
					}
					
					// 「脱出」は呼び出しノードに対してのみ有効
					EnableMenuItem( hPopupOfVar, IDM_VAR_STEPOUT, (VarTree::isCallNode(varname.c_str()) ? MFS_ENABLED : MFS_DISABLED) );
					
					// ポップアップメニューを表示する
					const int idSelected = TrackPopupMenuEx(
						hPopupOfVar, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
						(int)LOWORD(lp), (int)HIWORD(lp), hDlgWnd, NULL
					);
					
					switch ( idSelected ) {
						case IDM_VAR_LOGGING:
						{
							CString&& varinfoText = VarTree::getItemVarText( hVarTree, hItem );
							Knowbug::logmes( varinfoText.c_str() );		// logmes 送信
							return TRUE;
						}
						case IDM_VAR_UPDATE:
							TabVarsUpdate();
							break;
						case IDM_VAR_STEPOUT:		// 呼び出しノードと仮定してよい
						{
							auto const pCallInfo = reinterpret_cast<const ModcmdCallInfo*>( TreeView_GetItemLParam(hVarTree, hItem) );
							Knowbug::runStepOut( pCallInfo->sublev );	// 対象が実行される前まで進む
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
			NMHDR* nmhdr = (LPNMHDR)lp;
			
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
						if ( !g_config->bCustomDraw ) break;		// カスタムドローしない
						
						LRESULT res = VarTree::customDraw(
							hVarTree, reinterpret_cast<LPNMTVCUSTOMDRAW>(nmhdr)
						);
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
					if ( IsDlgButtonChecked( hLogPage, IDC_CHK_LOG_UPDATE ) ) goto LUpdate;
					break;
					
				case IDC_BTN_LOG_UPDATE:
				LUpdate:
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
		TCITEM tc;
		RECT rt;
		LPPOINT pt = (LPPOINT) &rt;
		
		// ポップアップメニューを生成
		hPopup   = CreatePopupMenu();
			AppendMenu( hPopup, (g_config->bTopMost ? MFS_CHECKED : MFS_UNCHECKED), IDM_TOPMOST, "常に最前面に表示する(&T)" );
		
		// ダイアログオブジェクトを生成
		HFONT hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
		hTabCtrl = GenerateObj( hDlg, WC_TABCONTROL, "", DIALOG_X0, DIALOG_Y0, DIALOG_X1, DIALOG_Y2, IDU_TAB, hFont );
		hSttCtrl = GenerateObj( hDlg, "static",    "",  DIALOG_X0 + 210, DIALOG_Y1 + 12, DIALOG_X1 - 210, 48, 0, hFont );
		hBtn1    = GenerateObj( hDlg, "button", "実行", DIALOG_X0 +   8, DIALOG_Y1 + 12, 40, 24, ID_BTN1, hFont );
		hBtn2    = GenerateObj( hDlg, "button", "次行", DIALOG_X0 +  48, DIALOG_Y1 + 12, 40, 24, ID_BTN2, hFont );	
		hBtn3    = GenerateObj( hDlg, "button", "停止", DIALOG_X0 +  88, DIALOG_Y1 + 12, 40, 24, ID_BTN3, hFont );
		hBtn4    = GenerateObj( hDlg, "button", "次飛", DIALOG_X0 + 128, DIALOG_Y1 + 12, 40, 24, ID_BTN4, hFont );		// 追加
		hBtn5    = GenerateObj( hDlg, "button", "脱出", DIALOG_X0 + 168, DIALOG_Y1 + 12, 40, 24, ID_BTN5, hFont );		// 追加
		
		// 全般タブを追加
		tc.mask = TCIF_TEXT;
		tc.pszText = "全般";
		TabCtrl_InsertItem(hTabCtrl, 0, &tc);
		hTabSheet[0] = CreateDialog( g_hInstance, "T_GENERAL", hDlg, (DLGPROC) TabGeneralProc );
		
		// 変数タブを追加
		tc.mask = TCIF_TEXT;
		tc.pszText = "変数";
		TabCtrl_InsertItem(hTabCtrl, 1, &tc);
		hTabSheet[1] = CreateDialog( g_hInstance, "T_VAR", hDlg, (DLGPROC) TabVarsProc );

		// ログタブを追加
		tc.mask    = TCIF_TEXT;
		tc.pszText = "ログ";
		TabCtrl_InsertItem(hTabCtrl, 2, &tc);
		hTabSheet[2] = CreateDialog( g_hInstance, "T_LOG", hDlg, (DLGPROC) TabLogProc );
		
		// スクリプトタブを追加
		tc.mask    = TCIF_TEXT;
		tc.pszText = "スクリプト";
		TabCtrl_InsertItem(hTabCtrl, 3, &tc);
		hTabSheet[3] = CreateDialog( g_hInstance, "T_SRC", hDlg, (DLGPROC) TabSrcProc );

		//GetClientRect(hTabCtrl, &rt);
		SetRect( &rt, 8, DIALOG_Y2 + 4, DIALOG_X1 + 8, DIALOG_Y1 + 4 );
		//TabCtrl_AdjustRect(hTabCtrl, FALSE, &rt);
		//MapWindowPoints(hTabCtrl, hDlg, pt, 2);

		// 生成した子ダイアログをタブシートの上に貼り付ける
		for ( int i = 0; i < TABDLGMAX; ++ i ) {
			MoveWindow(
				hTabSheet[i],
				rt.left,
				rt.top,
				rt.right  - rt.left,
				rt.bottom - rt.top,
				FALSE
			);
		}

		// デフォルトで左側のタブを表示
		ShowWindow( hTabSheet[0], SW_SHOW );
		return TRUE;
	}
	case WM_NOTIFY:
	{
		NMHDR *nm = (NMHDR *)lp;		// タブコントロールのシート切り替え通知
		int cur   = TabCtrl_GetCurSel(hTabCtrl);
		for ( int i = 0; i < TABDLGMAX; ++ i ) {
			ShowWindow( hTabSheet[i], (i == cur) ? SW_SHOW : SW_HIDE );
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
		const int idSelected = TrackPopupMenuEx(
			hPopup, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, hDlg, NULL
		);
		switch ( idSelected ) {
			case IDM_TOPMOST:
			{
				g_config->bTopMost = !g_config->bTopMost;		// 反転
				
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
		
#ifdef with_WrapCall
	case DWM_RequireDebugStruct:
	//	TabLogAdd("connected with WrapCall\n");
		return (LRESULT)( g_debug );
	case DWM_RequireMethodFunc:
	{
		WrapCall_RequireMethodFunc( reinterpret_cast<WrapCallMethod*>( lp ) );
		return 0;
	}
#endif
	/*
#ifdef with_Script
	case DWM_InitConnectWithScript: initConnectWithScript(); break;
	case DWM_SetNodeAnnotation: setNodeAnnotation( almighty_cast<const char*>(wp), almighty_cast<const char*>(lp) ); break;
	case DWM_SetStPrmNameBegin: setStPrmNameBegin( almighty_cast<const char*>(lp) ); break;
	case DWM_SetStPrmNameEnd:   setStPrmNameEnd(); break;
	case DWM_SetStPrmName:      setStPrmName( almighty_cast<int>(wp), almighty_cast<const char*>(lp) ); break;
#endif
	//*/
	}
	return DefWindowProc(hDlg, msg, wp, lp) ;
}

//------------------------------------------------
// メインダイアログを生成する
//------------------------------------------------
HWND Dialog::createMain()
{
	int dispx, dispy;
	WNDCLASS wndclass;

	dispx   = GetSystemMetrics( SM_CXSCREEN );
	dispy   = GetSystemMetrics( SM_CYSCREEN );
	
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = DlgProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = g_hInstance;
	wndclass.hIcon         = NULL;
	wndclass.hCursor       = LoadCursor(NULL,IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = myClass;
	RegisterClass( &wndclass );
	
	hDlgWnd = CreateWindow(
		myClass,
		"Debug Window",
		(WS_CAPTION | WS_OVERLAPPED | WS_BORDER | WS_VISIBLE),
		dispx - WND_SX, 0,
		WND_SX, WND_SY,
		NULL,
		NULL,
		g_hInstance,
		NULL
	);
	ShowWindow( hDlgWnd, SW_SHOW );
	UpdateWindow( hDlgWnd );
	
	// hDlgWnd = CreateDialog( myinst, "HSP3DEBUG", NULL, (DLGPROC)DlgProc );
	if ( hDlgWnd == NULL ) {
		MessageBox( NULL, "Debug window initalizing failed.", "Error", 0 );
	}
	
	SetWindowPos(	// 最前面
		hDlgWnd, (g_config->bTopMost ? HWND_TOPMOST : HWND_NOTOPMOST),
		0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)
	);
	
	ShowWindow( hDlgWnd, SW_SHOW );
    UpdateWindow( hDlgWnd );
	
	return hDlgWnd;
}

void Dialog::destroyMain()
{
	if ( hDlgWnd != NULL ) {
		DestroyWindow( hDlgWnd );
		hDlgWnd = NULL;
	}
	return;
}

//------------------------------------------------
// 更新
// 
// @ dbgnotice (stop) から呼ばれる。
//------------------------------------------------
void update()
{
	const int idxTab = TabCtrl_GetCurSel( hTabCtrl );
	switch( idxTab ) {
		case 0:
			TabGeneralReset();
			SrcSync( g_debug->fname, g_debug->line, false, true );
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
	Edit_EnableWordwrap( hEdit, g_config->bWordwrap );
	return;
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
			const int tabstops = tm.tmAveCharWidth / 4 * tabwidth * 2;
			SendMessage( hEdit, EM_SETTABSTOPS, 1, (LPARAM)(&tabstops) );
		}
	}
	ReleaseDC( hEdit, hdc );
	return;
}

//------------------------------------------------
// EditControl の「右端で折り返す」か否か
//------------------------------------------------
void Edit_EnableWordwrap( HWND hEdit, bool bEnable )
{
	LONG const Style_HorzScroll = WS_HSCROLL | ES_AUTOHSCROLL;
	LONG const style = GetWindowLongPtr(hEdit, GWL_STYLE);
	
	SetWindowLongPtr( hEdit, GWL_STYLE,
		g_config->bWordwrap ? (style &~ Style_HorzScroll) : (style | Style_HorzScroll)
	);
	SetWindowPos( hEdit, NULL, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED) );
	return;
}

//------------------------------------------------
// ツリービューの項目ラベルを取得する
//------------------------------------------------
CString TreeView_GetItemString( HWND hwndTree, HTREEITEM hItem )
{
	char stmp[256];
	
	TVITEM ti;
	ti.hItem      = hItem;
	ti.mask       = TVIF_TEXT;
	ti.pszText    = stmp;
	ti.cchTextMax = sizeof(stmp) - 1;
	
	if ( TreeView_GetItem( hwndTree, &ti ) ) {
		return stmp;
	} else {
		return "";
	}
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
		if ( hUpper == NULL ) hUpper = TreeView_GetParent(hwndTree, hItem);
		
		TreeView_SelectItem( hwndTree, hUpper );
	}
	return;
}
