
//
//		HSP debug window support functions for HSP3
//				onion software/onitama 2005
//

#pragma comment(lib, "comctl32.lib")

#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "module/hspdll.h"
#include "module/supio.h"
#include "module/SortNote.h"
#include "resource.h"
#include "main.h"

static HSP3DEBUG *g_debug;
static HWND hDlgWnd;
static HINSTANCE myinst;

#define IDU_TAB 100
#define ID_BTN1 1000
#define ID_BTN2 1001
#define ID_BTN3 1002
#define ID_BTN4 1003
#define ID_BTN5 1004

#define TABDLGMAX 3
#define myClass "KNOWBUG"

#define DIALOG_X0 5
#define DIALOG_Y0 5
#define DIALOG_X1 366
#define DIALOG_Y1 406
#define DIALOG_Y2 23

#define WND_SX 380
#define WND_SY 480

static HWND g_hTabCtrl;
static HWND g_hGenList;
static HWND g_hTabSheet[TABDLGMAX];

static HWND g_hVarPage;
static HWND g_hVarTree;
static HWND g_hVarEdit;

static HWND g_hBtn1;
static HWND g_hBtn2;
static HWND g_hBtn3;
static HWND g_hBtn4;
static HWND g_hBtn5;
static HWND g_hSttCtrl;

static HWND g_hLogPage;
static HWND g_hLogEdit;
static HWND g_hLogChkUpdate;
static HWND g_hLogChkCalog;

static HMENU g_hPopup;
static HMENU g_hPopupOfVar;

typedef BOOL (CALLBACK *HSP3DBGFUNC)(HSP3DEBUG *,int,int,int);

//----------------------------------------------------------

static HSPCTX*       ctx( NULL );
static HSPEXINFO* exinfo( NULL );

//----------------------------------------------------------

#include <deque>
#include <vector>
#include <list>
#include "module/mod_cstring.h"
#include "module/mod_cast.h"
#include "ClhspDebugInfo.h"
#include "CVarTree.h"
//#include "CVarinfoTree.h"
#include "CVarinfoText.h"
#include "CVarinfoLine.h"
#include "SysvarData.h"
#include "CIni.h"
#define dbgmsg(msg) MessageBox( NULL, msg, "Debug Window", MB_OK )

// グローバル変数
static DebugInfo  g_dbginfo_inst;
static DebugInfo* g_dbginfo      = &g_dbginfo_inst;
static CVarTree*  stt_pSttVarTree = NULL;
//static DynTree_t* stt_pDynTree  = NULL;
static CString*   stt_pLogmsg     = NULL;

static bool stt_bStepRunning = false;
static int stt_sublev_for_stepover = -1;

// 設定関連
static std::vector<COLORREF> stt_clrTypeText;
static bool stt_bCustomDraw   = false;
static int  stt_maxlenVarinfo = 0;
static int  stt_tabwidth      = 4;
static bool stt_bTopMost      = false;
static CString stt_logPath    = "";
static int  stt_logMaxlen     = 0x7FFF;
static bool stt_bWordwrap     = false;
static bool stt_bResultNode   = false;
static std::vector<CString> stt_extraTypeFormat;

static void getConfig( void );

static void setTabLength( HWND hEdit, const int tabwidth );
static void setWordwrapStyle( HWND hEdit, bool bEnable );

// 変数ツリー関連
static CVarTree* getSttVarTree( void );
static void VarTree_addNode( HWND hwndTree, HTREEITEM hParent, CVarTree& tree );
static void VarTree_addNodeSysvar( HWND hwndTree, HTREEITEM hParent );
static LRESULT   VarTree_customDraw( HWND hwndTree, LPNMTVCUSTOMDRAW pnmcd );
static vartype_t VarTree_getVartype( HWND hwndTree, HTREEITEM hItem );
static CString   VarTree_getItemVarText( HWND hwndTree, HTREEITEM hItem );

static PVal *seekSttVar( const char *name );
static vartype_t getVartype( const char *name );

static inline bool isModuleNode( const char *name ) { return name[0] == '@' || name[0] == '+'; }
static inline bool isSysvarNode( const char *name ) { return name[0] == '~'; }
static inline bool isCallNode  ( const char *name ) { return name[0] == '\''; }
static inline bool isResultNode( const char *name ) { return name[0] == '"'; }
static inline bool isVarNode   ( const char *name ) { return !(isModuleNode(name) || isSysvarNode(name) || isCallNode(name) || isResultNode(name)); }

static CString TreeView_GetItemString( HWND hwndTree, HTREEITEM hItem );
static LPARAM  TreeView_GetItemLParam( HWND hwndTree, HTREEITEM hItem );
static void    TreeView_EscapeFocus( HWND hwndTree, HTREEITEM hItem );

// ログ関連
static void TabLogSave( HWND hDlg, const char* filepath );

// ランタイムとの通信
EXPORT BOOL WINAPI debugini( HSP3DEBUG *p1, int p2, int p3, int p4 );
EXPORT BOOL WINAPI debug_notice( HSP3DEBUG *p1, int p2, int p3, int p4 );
#ifndef clhsp
EXPORT BOOL WINAPI debugbye( HSP3DEBUG *p1, int p2, int p3, int p4 );
#endif

// WrapCall 関連
#ifdef with_WrapCall
# include "../../../../../../MakeHPI/WrapCall/ModcmdCallInfo.h"
# include "../../../../../../MakeHPI/WrapCall/DbgWndMsg.h"
# include "../../../../../../MakeHPI/WrapCall/WrapCallSdk.h"

struct ResultNodeData {
	STRUCTDAT* pStDat;
	int sublev;
	CString valueString;						// 値の文字列化 (double, str, or int)
	const ModcmdCallInfo* pCallInfoDepended;	// これに依存する呼び出し (存在する場合はこれの子ノードになる)
};

static std::vector<const ModcmdCallInfo*> g_stkCallInfo;
static const ResultNodeData*              g_pLastResult = nullptr;
static HTREEITEM g_hNodeDynamic;

static void termNodeDynamic();

static void VarTree_addNodeDynamic( HWND hwndTree, HTREEITEM hParent );
static void CallTree_RemoveDependResult( HWND hwndTree, HTREEITEM hItem );

static void AddCallNode ( HWND hwndTree, const ModcmdCallInfo& callinfo );
static void RemoveCallNode( HWND hwndTree, const ModcmdCallInfo& callinfo );
static void UpdateCallNode( HWND hwndTree );
static void AddResultNode( HWND hwndTree, const ResultNodeData* pResult );
static void RemoveResultNode( HWND hwndTree, HTREEITEM hResult );

static void OnBgnCalling( HWND hwndTree, const ModcmdCallInfo& callinfo );
static void OnEndCalling( HWND hwndTree, const ModcmdCallInfo& callinfo );
static void OnResultReturning( HWND hwndTree, const ModcmdCallInfo& callinfo, void* ptr, int flag );

static void* ModcmdCallInfo_getPrmstk(const ModcmdCallInfo& callinfo);

// methods
static void WrapCallMethod_AddLog( const char* log );
static void WrapCallMethod_BgnCalling( unsigned int idx, const ModcmdCallInfo* pCallInfo );
static int  WrapCallMethod_EndCalling( unsigned int idx, const ModcmdCallInfo* pCallInfo );
static void WrapCallMethod_ResultReturning( unsigned int idx, const ModcmdCallInfo* pCallInfo, void* ptr, int flag );

#endif

// khad 関連
#ifdef with_khad
# include "khAd.h"
static HWND g_hKhad = NULL;
#endif

inline const char* STRUCTDAT_getName(const STRUCTDAT* pStDat) { return &ctx->mem_mds[pStDat->nameidx]; }

//----------------------------------------------------------

//------------------------------------------------
// Dllエントリーポイント
//------------------------------------------------
int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	switch ( fdwReason ) {
		case DLL_PROCESS_ATTACH:
			myinst = hInstance;
			hDlgWnd = NULL;
			break;
		
		case DLL_PROCESS_DETACH:
#ifndef clhsp
		//*
			debugbye( g_debug, 0, 0, 0 );
		/*/
			if ( stt_pSttVarTree ) {
				delete stt_pSttVarTree; stt_pSttVarTree = NULL;
			}
			if ( stt_pLogmsg ) {
				delete stt_pLogmsg; stt_pLogmsg = NULL;
			}
		//*/
#endif
			if ( hDlgWnd != NULL ) {
				DestroyWindow( hDlgWnd );
				hDlgWnd = NULL;
			}
			break;
	}
	return TRUE;
}

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
		myinst,
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
	ListView_InsertColumn( g_hGenList , 0 , &col);
	
	col.cx       = 400;
	col.iSubItem = 1;
	col.pszText  = "内容";
	ListView_InsertColumn( g_hGenList , 1 , &col);
	return;
}

//------------------------------------------------
// 全般タブの更新
//------------------------------------------------
static void TabGeneral_AddItem( const char *sItem, const char *sValue, int iItem );

static void TabGeneralReset( void )
{
	int chk, tgmax;
	char *p;
	char name[256];
	char val[512];

	ListView_DeleteAllItems( g_hGenList );
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
	do if ( g_dbginfo->exinfo->actscr != NULL ) {
		BMSCR *pBmscr(
			ptr_cast<BMSCR *>( g_dbginfo->exinfo->HspFunc_getbmscr( *g_dbginfo->exinfo->actscr ) )
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
	return;
}

static void TabGeneral_AddItem( const char *sItem, const char *sValue, int iItem )
{
	LV_ITEM item;
	
	item.mask     = LVIF_TEXT;
	item.iItem    = iItem;
	
	item.iSubItem = 0;
	item.pszText  = const_cast<char *>(sItem);
	ListView_InsertItem( g_hGenList, &item );
	
	item.iSubItem = 1;
	item.pszText  = const_cast<char *>(sValue);
	ListView_SetItem( g_hGenList, &item );
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
	SetWindowText( g_hSttCtrl, tmp );
	
#ifdef with_khad
	if ( g_hKhad != NULL ) {
		SendMessage( g_hKhad, UWM_KHAD_CURPOS, g_debug->line, (LPARAM)g_debug->fname );
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
	if ( IsDlgButtonChecked( g_hVarPage, IDC_MODULE ) ) opt |= 2;
	if ( IsDlgButtonChecked( g_hVarPage, IDC_ARRAY  ) ) opt |= 4;
	if ( IsDlgButtonChecked( g_hVarPage, IDC_DUMP   ) ) opt |= 8;
	return opt;
//*/
}

//------------------------------------------------
// 変数タブの更新
//------------------------------------------------
static void TabVarsReset( void )
{
	TreeView_DeleteAllItems( g_hVarTree );
	
	// 静的変数リストをツリーに追加する
	CVarTree* pVarTree = getSttVarTree();
	
	VarTree_addNode       ( g_hVarTree, TVI_ROOT, *pVarTree );
#ifdef with_WrapCall
	VarTree_addNodeDynamic( g_hVarTree, TVI_ROOT );
#endif
	VarTree_addNodeSysvar ( g_hVarTree, TVI_ROOT );
	
	// すべてのルートノードを開く
	HTREEITEM hRoot( TreeView_GetRoot(g_hVarTree) );
	
	for ( HTREEITEM hNode = hRoot
		; hNode != NULL
		; hNode = TreeView_GetNextSibling( g_hVarTree, hNode )
	) {
		TreeView_Expand( g_hVarTree, hNode, TVE_EXPAND );
	}
	
	// トップを表示するようにし向ける
	TreeView_EnsureVisible( g_hVarTree, hRoot );
	
	return;
}

//------------------------------------------------
// 変数タブ::変数情報の更新
//------------------------------------------------
static void TabVarsUpdate( void )
{
	HTREEITEM hItem = TreeView_GetSelection( g_hVarTree );
	if ( hItem == NULL ) return;
	
	CString&& varinfoText = VarTree_getItemVarText( g_hVarTree, hItem );
	if ( !varinfoText.empty() ) {
		SetWindowText( g_hVarEdit, varinfoText.c_str() );
	}
	return;
}

//------------------------------------------------
// ログメッセージを追加・更新する
//------------------------------------------------
static void TabLogAdditionalUpdate( const char *text )
{
	int caret[2];
	SendMessage( g_hLogEdit, EM_GETSEL,
		(WPARAM)( &caret[0] ),
		(LPARAM)( &caret[1] )
	);
	
	int size = Edit_GetTextLength( g_hLogEdit );
	Edit_SetSel( g_hLogEdit, size, size );		// 最後尾にキャレットを置く
	Edit_ReplaceSel( g_hLogEdit, text );		// 文字列(str)を追加する
	Edit_ScrollCaret( g_hLogEdit );				// 画面を必要なだけスクロール
	
	// 選択状態を元に戻す
	Edit_SetSel( g_hLogEdit, caret[0], caret[1] );
	return;
}

//------------------------------------------------
// ログメッセージを更新する (commit)
//------------------------------------------------
static void TabLogCommit( void )
{
	if ( stt_pLogmsg == NULL || stt_pLogmsg->empty() ) return;
	
	TabLogAdditionalUpdate( stt_pLogmsg->c_str() );
	stt_pLogmsg->clear();
	return;
}

//------------------------------------------------
// ログメッセージに追加する
//------------------------------------------------
static void TabLogAdd( const char *str )
{
	// 自動更新
	if ( IsDlgButtonChecked( g_hLogPage, IDC_CHK_UPDATE ) ) {
		
		TabLogAdditionalUpdate( str );
		
	} else {
		if ( stt_pLogmsg == NULL ) {
			stt_pLogmsg = new CString;
			stt_pLogmsg->reserve( 0x400 + 1 );
		}
		
		stt_pLogmsg->append( str );
	}
	
	return;
}

static void TabLogAddCrlf()
{
	TabLogAdd( "\r\n" );
}

//------------------------------------------------
// ログメッセージを初期化する
//------------------------------------------------
static void TabLogClear()
{
	if ( stt_pLogmsg != NULL ) stt_pLogmsg->clear();
	
	Edit_SetSel( g_hLogEdit, 0, -1 );		// 最後尾にキャレットを向ける
	Edit_ReplaceSel( g_hLogEdit, "" );		// 空っぽにする
	return;
}

//------------------------------------------------
// ログメッセージを保存する
//------------------------------------------------
static void TabLogSave( HWND hDlg )
{
	char filename[MAX_PATH + 1] = "";
	char fullname[MAX_PATH + 1] = "hspdbg.log";
	OPENFILENAME ofn = { 0 };
		ofn.lStructSize    = sizeof(ofn);			// 構造体のサイズ
		ofn.hwndOwner      = hDlg;					// コモンダイアログの親ウィンドウハンドル
		ofn.lpstrFilter    = "log text(*.txt;*.log)\0*.txt;*.log\0All files(*.*)\0*.*\0\0";	// ファイルの種類
		ofn.lpstrFile      = fullname;				// 選択されたファイル名(フルパス)を受け取る変数のアドレス
		ofn.lpstrFileTitle = filename;				// 選択されたファイル名を受け取る変数のアドレス
		ofn.nMaxFile       = sizeof(fullname);		// lpstrFileに指定した変数のサイズ
		ofn.nMaxFileTitle  = sizeof(filename);		// lpstrFileTitleに指定した変数のサイズ
		ofn.Flags          = OFN_OVERWRITEPROMPT;	// フラグ指定
		ofn.lpstrTitle     = "名前を付けて保存";	// コモンダイアログのキャプション
		ofn.lpstrDefExt    = "log";					// デフォルトのファイルの種類

	if ( !GetSaveFileName( &ofn ) ) return;
	
	TabLogSave( hDlg, fullname );
	return;
}

static void TabLogSave( HWND hDlg, const char* filepath )
{
	// ログメッセージを取り出す
	int size = Edit_GetTextLength( g_hLogEdit );
	char* buf = new char[size + 2];
	GetWindowText( g_hLogEdit, buf, size + 1 );
	
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
// 全般タブ::プロシージャ
//------------------------------------------------
LRESULT CALLBACK TabGeneralProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch ( msg ) {
		
		case WM_INITDIALOG:
			g_hGenList = GetDlgItem( hDlg, IDC_LV_GENERAL );
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
			g_hVarPage = hDlg;
			g_hVarTree = GetDlgItem( hDlg, IDC_VARTREE );
			g_hVarEdit = GetDlgItem( hDlg, IDC_VARINFO );
			
			setTabLength( g_hVarEdit, stt_tabwidth );	// タブ文字の長さ
			setWordwrapStyle( g_hVarEdit, stt_bWordwrap );
			
			TabVarsReset();
			
			// ポップアップメニューの追加
			g_hPopupOfVar = CreatePopupMenu();
				AppendMenu( g_hPopupOfVar, MF_STRING, IDM_VAR_LOGGING, "ログ(&L)");	// 文は表示時に上書きされる
				AppendMenu( g_hPopupOfVar, MF_STRING, IDM_VAR_UPDATE,  "更新(&U" );
				AppendMenu( g_hPopupOfVar, MF_SEPARATOR, 0, 0 );
				AppendMenu( g_hPopupOfVar, MF_STRING, IDM_VAR_STEPOUT, "脱出(&O)" );
			return TRUE;
			
		case WM_CONTEXTMENU:
		{
			// ツリー上で逆クリック
			if ( wp == (WPARAM)g_hVarTree ) {
				TV_HITTESTINFO tvHitTestInfo;
					tvHitTestInfo.pt.x = LOWORD(lp);
					tvHitTestInfo.pt.y = HIWORD(lp);
				ScreenToClient( g_hVarTree, &tvHitTestInfo.pt );
				const auto hItem = TreeView_HitTest( g_hVarTree, &tvHitTestInfo );	// 対象を確定
				if ( hItem == NULL )  break;
				
				if ( tvHitTestInfo.flags & TVHT_ONITEMLABEL ) {		// 文字列アイテムの場合
					const auto varname = TreeView_GetItemString(g_hVarTree, hItem);
					{
						const auto menuText = strf( "「%s」をログ(&L)", varname.c_str() );
						MENUITEMINFO menuInfo;
							menuInfo.cbSize = sizeof(menuInfo);
							menuInfo.fMask  = MIIM_STRING;
							menuInfo.dwTypeData = const_cast<LPSTR>( menuText.c_str() );
						SetMenuItemInfo( g_hPopupOfVar, IDM_VAR_LOGGING, FALSE, &menuInfo );
					}
					
					// 「脱出」は呼び出しノードに対してのみ有効
					EnableMenuItem( g_hPopupOfVar, IDM_VAR_STEPOUT, (isCallNode(varname.c_str()) ? MFS_ENABLED : MFS_DISABLED) );
					
					// ポップアップメニューを表示する
					const int idSelected = TrackPopupMenuEx(
						g_hPopupOfVar, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
						(int)LOWORD(lp), (int)HIWORD(lp), hDlgWnd, NULL
					);
					
					switch ( idSelected ) {
						case IDM_VAR_LOGGING:
						{
							CString&& varinfoText = VarTree_getItemVarText( g_hVarTree, hItem );
							TabLogAdd( varinfoText.c_str() );		// logmes 送信
							return TRUE;
						}
						case IDM_VAR_UPDATE:
							TabVarsUpdate();
							break;
						case IDM_VAR_STEPOUT:		// 呼び出しノードと仮定してよい
						{
							auto const pCallInfo = almighty_cast<const ModcmdCallInfo*>( TreeView_GetItemLParam(g_hVarTree, hItem) );
							stt_sublev_for_stepover = pCallInfo->sublev;	// 対象が実行される前まで進む
							g_debug->dbg_set( HSPDEBUG_STEPIN );
							SetWindowText( g_hSttCtrl, "" );
							stt_bStepRunning = true;
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
			
			if ( nmhdr->hwndFrom == g_hVarTree ) {
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
						if ( !stt_bCustomDraw ) break;		// カスタムドローしない
						
						LRESULT res = VarTree_customDraw(
							g_hVarTree, ptr_cast<LPNMTVCUSTOMDRAW>(nmhdr)
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
			g_hLogPage = hDlg;
			g_hLogEdit = GetDlgItem( hDlg, IDC_LOG );
			g_hLogChkUpdate = GetDlgItem( hDlg, IDC_CHK_UPDATE );
			g_hLogChkCalog  = GetDlgItem( hDlg, IDC_CHK_CALOG );
			
			CheckDlgButton( g_hLogPage, IDC_CHK_UPDATE, BST_CHECKED );
#ifdef with_WrapCall
		//	CheckDlgButton( g_hLogPage, IDC_CHK_CALOG,  BST_CHECKED );
#else
			EnableWindow( g_hLogChkCalog, false );
#endif
			
			setTabLength( g_hLogEdit, stt_tabwidth );
			setWordwrapStyle( g_hLogEdit, stt_bWordwrap );
			SendMessage( g_hLogEdit, EM_SETLIMITTEXT, (WPARAM) stt_logMaxlen, 0 );
			return TRUE;
			
		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
				
				case IDC_CHK_UPDATE:
					// チェックが付けられたとき
					if ( IsDlgButtonChecked( g_hLogPage, IDC_CHK_UPDATE ) ) {
						TabLogCommit();
					}
					break;
					
				case IDC_BTN_LOG_UPDATE:
					TabLogCommit();
					break;
					
				case IDC_BTN_LOG_SAVE:
					TabLogSave( hDlg );
					break;
					
				case IDC_BTN_LOG_CLEAR:
					TabLogClear();
					break;
			}
			return FALSE;
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
		g_hPopup   = CreatePopupMenu();
			AppendMenu( g_hPopup, (stt_bTopMost ? MFS_CHECKED : MFS_UNCHECKED), IDM_TOPMOST, "常に最前面に表示する(&T)" );
		
		// ダイアログオブジェクトを生成
		HFONT hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
		g_hTabCtrl = GenerateObj( hDlg, WC_TABCONTROL, "", DIALOG_X0, DIALOG_Y0, DIALOG_X1, DIALOG_Y2, IDU_TAB, hFont );
		g_hSttCtrl = GenerateObj( hDlg, "static",    "",  DIALOG_X0 + 210, DIALOG_Y1 + 4, DIALOG_X1 - 210, 48, 0, hFont );
		g_hBtn1    = GenerateObj( hDlg, "button", "実行", DIALOG_X0 +   8, DIALOG_Y1 + 4, 40, 24, ID_BTN1, hFont );
		g_hBtn2    = GenerateObj( hDlg, "button", "次行", DIALOG_X0 +  48, DIALOG_Y1 + 4, 40, 24, ID_BTN2, hFont );	
		g_hBtn3    = GenerateObj( hDlg, "button", "停止", DIALOG_X0 +  88, DIALOG_Y1 + 4, 40, 24, ID_BTN3, hFont );
		g_hBtn4    = GenerateObj( hDlg, "button", "次飛", DIALOG_X0 + 128, DIALOG_Y1 + 4, 40, 24, ID_BTN4, hFont );		// 追加
		g_hBtn5    = GenerateObj( hDlg, "button", "脱出", DIALOG_X0 + 168, DIALOG_Y1 + 4, 40, 24, ID_BTN5, hFont );		// 追加
		
		// 全般タブを追加
		tc.mask = TCIF_TEXT;
		tc.pszText = "全般";
		TabCtrl_InsertItem(g_hTabCtrl, 0, &tc);
		g_hTabSheet[0] = CreateDialog( myinst, "T_GENERAL", hDlg, (DLGPROC) TabGeneralProc );
		
		// 変数タブを追加
		tc.mask = TCIF_TEXT;
		tc.pszText = "変数";
		TabCtrl_InsertItem(g_hTabCtrl, 1, &tc);
		g_hTabSheet[1] = CreateDialog( myinst, "T_VAR", hDlg, (DLGPROC) TabVarsProc );

		// ログタブを追加
		tc.mask    = TCIF_TEXT;
		tc.pszText = "ログ";
		TabCtrl_InsertItem(g_hTabCtrl, 2, &tc);
		g_hTabSheet[2] = CreateDialog( myinst, "T_LOG", hDlg, (DLGPROC) TabLogProc );
		
		//GetClientRect(g_hTabCtrl, &rt);
		SetRect( &rt, 8, DIALOG_Y2 + 4, DIALOG_X1 + 8, DIALOG_Y1 + 4 );
		//TabCtrl_AdjustRect(g_hTabCtrl, FALSE, &rt);
		//MapWindowPoints(g_hTabCtrl, hDlg, pt, 2);

		// 生成した子ダイアログをタブシートの上に貼り付ける
		for ( int i = 0; i < TABDLGMAX; ++ i ) {
			MoveWindow(
				g_hTabSheet[i],
				rt.left,
				rt.top,
				rt.right  - rt.left,
				rt.bottom - rt.top,
				FALSE
			);
		}

		// デフォルトで左側のタブを表示
		ShowWindow( g_hTabSheet[0], SW_SHOW );
		return TRUE;
	}
	case WM_NOTIFY:
	{
		NMHDR *nm = (NMHDR *)lp;		// タブコントロールのシート切り替え通知
		int cur   = TabCtrl_GetCurSel(g_hTabCtrl);
		for ( int i = 0; i < TABDLGMAX; ++ i ) {
			ShowWindow( g_hTabSheet[i], (i == cur) ? SW_SHOW : SW_HIDE );
		}
		break;
	}
	case WM_COMMAND:
		switch ( LOWORD(wp) ) {
			case ID_BTN1: LRun:
				g_debug->dbg_set( HSPDEBUG_RUN );
				SetWindowText( g_hSttCtrl, "" );
				stt_bStepRunning = false;
				break;
			case ID_BTN2: LStepIn:
				g_debug->dbg_set( HSPDEBUG_STEPIN );
				SetWindowText( g_hSttCtrl, "" );
				stt_bStepRunning = true;
				break;
				//*
			case ID_BTN3:
				g_debug->dbg_set( HSPDEBUG_STOP );
				break;
			case ID_BTN4:
			//	g_debug->dbg_set( HSPDEBUG_STEPOVER ); break;
				stt_sublev_for_stepover = ctx->sublev;
				goto LStepIn;
				//*/
			case ID_BTN5:
			//	g_debug->dbg_set( HSPDEBUG_STEPOUT ); break;
				if ( ctx->sublev == 0 ) goto LRun;			// 最外周からの脱出 = 無制限
				stt_sublev_for_stepover = ctx->sublev - 1;
				goto LStepIn;
		}
		break;
		
	case WM_CONTEXTMENU:		// ポップアップメニュー表示
	{
		POINT pt;
		GetCursorPos( &pt );	// カーソル位置 (スクリーン座標)
		
		// ポップアップメニューを表示する
		const int idSelected = TrackPopupMenuEx(
			g_hPopup, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, hDlg, NULL
		);
		switch ( idSelected ) {
			case IDM_TOPMOST:
			{
				stt_bTopMost = !stt_bTopMost;		// 反転
				
				MENUITEMINFO menuInfo;
					menuInfo.cbSize = sizeof(menuInfo);
					menuInfo.fMask  = MIIM_STATE;
					menuInfo.fState = ( stt_bTopMost ? MFS_CHECKED : MFS_UNCHECKED );
				SetMenuItemInfo( g_hPopup, IDM_TOPMOST, FALSE, &menuInfo );
					
				SetWindowPos(	// 最前面
					hDlgWnd, (stt_bTopMost ? HWND_TOPMOST : HWND_NOTOPMOST),
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
		DestroyMenu( g_hPopup );      g_hPopup      = nullptr;
		DestroyMenu( g_hPopupOfVar ); g_hPopupOfVar = nullptr;
		PostQuitMessage(0);
		break;
		
#ifdef with_WrapCall
	case DWM_RequireDebugStruct:
	//	TabLogAdd("connected with WrapCall\n");
		return almighty_cast<LRESULT>( g_debug );
	case DWM_RequireMethodFunc:
	{
		WrapCallMethod* methods = almighty_cast<WrapCallMethod*>( lp );
		methods->AddLog          = WrapCallMethod_AddLog;
		methods->BgnCalling      = WrapCallMethod_BgnCalling;
		methods->EndCalling      = WrapCallMethod_EndCalling;
		methods->ResultReturning = WrapCallMethod_ResultReturning;
		return 0;
	}
#endif
		
#ifdef with_khad
	case UWM_KHAD_GREET:
		g_hKhad = (HWND)lp;
		SendMessage( g_hKhad, UWM_KHAD_GREET, 0, (LPARAM)hDlg );
		break;
	case UWM_KHAD_BYE:
		g_hKhad = NULL;
		break;
	case UWM_KHAD_ADDLOG:
		sprintf( ctx->stmp, "%1020s\r\n", almighty_cast<const char*>( lp ) );
		TabLogAdd( ctx->stmp );
		break;
#endif
	}
	return DefWindowProc(hDlg, msg, wp, lp) ;
}

//##############################################################################
//        デバッグウィンドウ::(runtime から呼ばれる関数)
//##############################################################################
//------------------------------------------------
// debugini ptr  (type1)
//------------------------------------------------
EXPORT BOOL WINAPI debugini( HSP3DEBUG *p1, int p2, int p3, int p4 )
{
	// 設定を読み込む
	getConfig();
	
//	DynTree::g_dbginfo = g_dbginfo;
	
	// ウィンドウの生成
	int dispx, dispy;
	WNDCLASS wndclass;

	g_debug = p1;
	ctx     = p1->hspctx;
	exinfo  = ctx->exinfo2;
	
	g_dbginfo->debug  = p1;
	g_dbginfo->ctx    = p1->hspctx;
	g_dbginfo->exinfo = g_dbginfo->ctx->exinfo2;
	
	dispx   = GetSystemMetrics( SM_CXSCREEN );
	dispy   = GetSystemMetrics( SM_CYSCREEN );
	
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = DlgProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = myinst;
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
		myinst,
		NULL
	);
	ShowWindow( hDlgWnd, SW_SHOW );
	UpdateWindow( hDlgWnd );
	
	// hDlgWnd = CreateDialog( myinst, "HSP3DEBUG", NULL, (DLGPROC)DlgProc );
	if ( hDlgWnd == NULL ) {
		MessageBox( NULL, "Debug window initalizing failed.", "Error", 0 );
	}
	
	SetWindowPos(	// 最前面
		hDlgWnd, (stt_bTopMost ? HWND_TOPMOST : HWND_NOTOPMOST),
		0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)
	);
	
	ShowWindow( hDlgWnd, SW_SHOW );
    UpdateWindow( hDlgWnd );
	
#ifdef with_WrapCall
	g_dbginfo->exinfo->er = almighty_cast<int*>( hDlgWnd );
#endif
	
	return 0;
}

//------------------------------------------------
// debug_notice ptr  (type1)
// 
// @prm p2 : 0 = stop event,
// @       : 1 = send message (logmes)
//------------------------------------------------
EXPORT BOOL WINAPI debug_notice( HSP3DEBUG *p1, int p2, int p3, int p4 )
{
	switch ( p2 ) {
		// 実行が停止した (stop, wait, await, assert など)
		case DebugNotice_Stop:
		{
			if ( stt_sublev_for_stepover >= 0 ) {
				if ( ctx->sublev > stt_sublev_for_stepover ) {
					g_debug->dbg_set( HSPDEBUG_STEPIN );		// stepin を繰り返す
					break;
				} else {
					stt_sublev_for_stepover = -1;	// 終了
				//	g_debug->dbg_set( HSPDEBUG_STOP );
				}
			}
			
#ifdef with_WrapCall
			UpdateCallNode( g_hVarTree );		// 呼び出しノード更新
#endif
			
			CurrnetUpdate();
			int idxTab = TabCtrl_GetCurSel(g_hTabCtrl);
			switch( idxTab ) {
				case 0:
					TabGeneralReset();
					break;
				case 1:
					TabVarsUpdate();
					break;
			}
			break;
		}
		
		// logmes 命令が呼ばれた
		case DebugNotice_Logmes:
			TabLogAdd( ctx->stmp );
			TabLogAddCrlf();
			break;
	}
	return 0;
}

//------------------------------------------------
// debugbye ptr  (type1)
// 
// @ clhsp からは呼ばれるが、hsp なら自分で呼ぶ。
//------------------------------------------------
EXPORT BOOL WINAPI debugbye( HSP3DEBUG *p1, int p2, int p3, int p4 )
{
	if ( !stt_logPath.empty() ) {		// 自動ログ保存
		TabLogSave( hDlgWnd, stt_logPath.c_str() );
	}
	
#ifdef with_khad
	if ( g_hKhad ) SendMessage( g_hKhad, UWM_KHAD_BYE, 0, 0 );
#endif
#ifdef with_WrapCall
	if ( g_hNodeDynamic ) termNodeDynamic();
#endif
	if ( stt_pSttVarTree ) {
		delete stt_pSttVarTree; stt_pSttVarTree = NULL;
	}
	if ( stt_pLogmsg ) {
		delete stt_pLogmsg; stt_pLogmsg = NULL;
	}
	return 0;
}

//##############################################################################
//               下請け関数
//##############################################################################
//------------------------------------------------
// ini ファイルからの読み込み
//------------------------------------------------
static void getConfig( void )
{
	char ownpath[MAX_PATH];
	{
		GetModuleFileName( GetModuleHandle(NULL), ownpath, MAX_PATH );
		
		char drive[5];
		char dir[MAX_PATH];
		char _dummy[MAX_PATH];		// ダミー
		
		_splitpath_s( ownpath, drive, dir, _dummy, _dummy );
		sprintf_s( ownpath, "%s%s", drive, dir );
	}
	
	CString const ownpath_full = strf("%sknowbug.ini", ownpath);
	CIni ini( ownpath_full.c_str() );
	
	// カスタムドローするかどうか
	stt_bCustomDraw = ini.getBool( "ColorType", "bCustomDraw", false );
	
	char stmp[256] = "";
	
	// 型ごとの色
	stt_clrTypeText.reserve( HSPVAR_FLAG_USERDEF );
	
	for ( int i = 0; i < HSPVAR_FLAG_USERDEF; ++ i ) {
		sprintf_s( stmp, "text#%d", i );
		stt_clrTypeText.push_back(
			ini.getInt( "ColorType", stmp, RGB(0, 0, 0) )
		);
	}
	
	// 最大表示データ量
	stt_maxlenVarinfo = ini.getInt( "Varinfo", "maxlen", 0x1000 - 1 );
	stt_logMaxlen     = ini.getInt( "Log",     "maxlen", 0x20000 );
	
	// タブ文字幅
	stt_tabwidth  = ini.getInt( "Interface", "tabwidth", 4 );
	
	// 右端で折り返すか否か
	stt_bWordwrap = ini.getBool( "Interface", "bWordwrap", false );
	
	// 最前面ウィンドウか否か
	stt_bTopMost = ini.getBool( "Window", "bTopMost", false );
	
	// 自動保存パス
	stt_logPath = ini.getString( "Log", "autoSavePath", "" );
	
	// 返値ノードを使うか
	stt_bResultNode = ini.getBool( "Varinfo", "useResultNode", false );
	
	return;
}

//------------------------------------------------
// EditControl のタブ文字幅を変更する
//------------------------------------------------
static void setTabLength( HWND hEdit, const int tabwidth )
{
	HDC const hdc = GetDC(hEdit);
	{
		TEXTMETRIC tm;
		if ( GetTextMetrics(hdc, &tm) ) {
			const int tabstops = tm.tmAveCharWidth / 4 * tabwidth * 2;
			SendMessage( hEdit, EM_SETTABSTOPS, 1, almighty_cast<LPARAM>(&tabstops) );
		}
	}
	ReleaseDC(g_hVarEdit, hdc);
	return;
}

//------------------------------------------------
// EditControl の「右端で折り返す」か否か
//------------------------------------------------
static void setWordwrapStyle( HWND hEdit, bool bEnable )
{
	LONG const Style_HorzScroll = WS_HSCROLL | ES_AUTOHSCROLL;
	LONG const style = GetWindowLongPtr(hEdit, GWL_STYLE);
	
	SetWindowLongPtr( hEdit, GWL_STYLE,
		stt_bWordwrap ? (style &~ Style_HorzScroll) : (style | Style_HorzScroll)
	);
	SetWindowPos( hEdit, NULL, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED) );
	return;
}

//------------------------------------------------
// ツリービューの項目ラベルを取得する
//------------------------------------------------
static CString TreeView_GetItemString( HWND hwndTree, HTREEITEM hItem )
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
static LPARAM TreeView_GetItemLParam( HWND hwndTree, HTREEITEM hItem )
{
	TVITEM ti;
	ti.hItem = hItem;
	ti.mask  = TVIF_PARAM;
	
	TreeView_GetItem( hwndTree, &ti );
	return ti.lParam;
}

//------------------------------------------------
// ツリービューのフォーカスを回避する
// @public
// @ 削除するノードが選択状態なら、その兄ノードか親ノードを選択する)
//------------------------------------------------
static void TreeView_EscapeFocus( HWND hwndTree, HTREEITEM hItem )
{
	if ( TreeView_GetSelection(hwndTree) == hItem ) {
		HTREEITEM hUpper = TreeView_GetPrevSibling( hwndTree, hItem );
		if ( hUpper == NULL ) hUpper = TreeView_GetParent(hwndTree, hItem);
		
		TreeView_SelectItem( hwndTree, hUpper );
	}
	return;
}

//------------------------------------------------
// 静的変数リストを取得する
//------------------------------------------------
static CVarTree *getSttVarTree( void )
{
	CVarTree*& vartree = stt_pSttVarTree;
	
	// 変数リストを作る
	if ( vartree == NULL ) {
		
		char name[0x100];
		
		// 静的変数リストを取得する
		char *p = g_debug->get_varinf( NULL, 0xFF );	// HSP側に問い合わせ
	//	SortNote( p );			// (-) ツリービュー側でソートするので不要
		
		vartree = new CVarTree( "", CVarTree::NodeType_Module );
		
		strsp_ini();
		for (;;) {
			int chk = strsp_get( p, name, 0, 255 );
			if ( chk == 0 ) break;
			
			vartree->push_var( name );
		}
		
		g_debug->dbg_close( p );
	}
	
	return vartree;
}

//------------------------------------------------
// 変数ツリーに要素を追加する
//------------------------------------------------
static void VarTree_addNode( HWND hwndTree, HTREEITEM hParent, CVarTree& tree )
{
	TVINSERTSTRUCT tvis;
	tvis.hParent      = hParent;
	tvis.hInsertAfter = TVI_SORT;
	tvis.item.mask    = TVIF_TEXT;
	
	switch ( tree.getType() )
	{
		// 変数ノード
		case CVarTree::NodeType_Var:
		{
			tvis.item.pszText = const_cast<char*>( tree.getName().c_str() );
			hParent = TreeView_InsertItem( hwndTree, &tvis );
			break;
		}
		
		// モジュール・ノード
		case CVarTree::NodeType_Module:
		{
			CString modname( "@" );
			modname.append( tree.getName() );
			
			tvis.item.pszText = const_cast<char*>( modname.c_str() );
			hParent = TreeView_InsertItem( hwndTree, &tvis );
			
			// それぞれの子ノードについて再帰処理
			for ( CVarTree::iterator iter = tree.begin()
				; iter != tree.end()
				; ++ iter
			) {
				VarTree_addNode( hwndTree, hParent, *iter );
			}
			
			break;
		}
	}
	
	return;
}

//------------------------------------------------
// 変数ツリーにシステム変数ノードを追加する
//------------------------------------------------
static void VarTree_addNodeSysvar( HWND hwndTree, HTREEITEM hParent )
{
	TVINSERTSTRUCT tvis;
	tvis.hParent      = hParent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask    = TVIF_TEXT;
	tvis.item.pszText = "+sysvar";
	
	HTREEITEM hNodeSysvar = TreeView_InsertItem( hwndTree, &tvis );
	
	tvis.hParent      = hNodeSysvar;
	tvis.hInsertAfter = TVI_LAST;		// 順番を守る
	
	// システム変数のリストを追加する
	for ( int i = 0; i < SysvarCount; ++ i ) {
		CString name = strf( "~%s", SysvarData[i].name );
		
		tvis.item.pszText = const_cast<char *>( name.c_str() );
		
		TreeView_InsertItem( hwndTree, &tvis );
	}
	
	return;
}

#ifdef with_WrapCall
//------------------------------------------------
// 変数ツリーに動的変数ノードを追加する
//------------------------------------------------
static void VarTree_addNodeDynamic( HWND hwndTree, HTREEITEM hParent )
{
	TVINSERTSTRUCT tvis;
	tvis.hParent      = hParent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask    = TVIF_TEXT;
	tvis.item.pszText = "+dynamic";
	
	g_hNodeDynamic = TreeView_InsertItem( hwndTree, &tvis );
	
//	tvis.hParent      = g_hNodeDynamic;
//	tvis.hInsertAfter = TVI_LAST;		// 順番を守る
	
	return;
}
#endif

//------------------------------------------------
// 変数ツリーの NM_CUSTOMDRAW を処理する
//------------------------------------------------
static LRESULT VarTree_customDraw( HWND hwndTree, LPNMTVCUSTOMDRAW pnmcd )
{
	if ( pnmcd->nmcd.dwDrawStage == CDDS_PREPAINT ) {
		return CDRF_NOTIFYITEMDRAW;
		
	} else if ( pnmcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT ) {
		
		HTREEITEM hItem ( almighty_cast<HTREEITEM>( pnmcd->nmcd.dwItemSpec ) );//( TreeView_GetSelection(hwndTree) );
		CString   sItem ( TreeView_GetItemString( hwndTree, hItem ) );
		const char *name ( sItem.c_str() );
		
		if ( isModuleNode(name) ) {
		//	pnmcd->clrText   = stt_clrTypeText[HSPVAR_FLAG_NONE];
		//	pnmcd->clrText   = RGB(   0,   0,   0 );
		//	pnmcd->clrTextBk = RGB( 255, 255, 255 );
			return 0;
			
		} else /* if ( isSysvarNode(name) or isSttVarNode(name) ) */ {
			vartype_t vtype ( getVartype(name) );
			
			// 組み込み型
			if ( 0 < vtype && vtype < HSPVAR_FLAG_USERDEF ) {
				pnmcd->clrText   = stt_clrTypeText[vtype];
				
			// 拡張型
			} else {
				pnmcd->clrText   = stt_clrTypeText[0];
			}
		}
		return CDRF_NEWFONT;
	}
	
	return 0;
}

//------------------------------------------------
// 変数ツリーの要素の型を取得する
//------------------------------------------------
static vartype_t VarTree_getVartype( HWND hwndTree, HTREEITEM hItem )
{
	CString    sItem( TreeView_GetItemString( hwndTree, hItem ) );
	const char *name( sItem.c_str() );
	
	return getVartype( name );
}

//------------------------------------------------
// 静的変数の PVal を求める
//------------------------------------------------
static PVal *seekSttVar( const char *name )
{
	int iVar( g_dbginfo->exinfo->HspFunc_seekvar( name ) );
	
	if ( iVar < 0 ) {
		return NULL;
	} else {
		return &g_dbginfo->ctx->mem_var[iVar];
	}
}

//------------------------------------------------
// 変数の型を取得する
//------------------------------------------------
static vartype_t getVartype( const char *name )
{
	// モジュールノード
	if ( isModuleNode(name) ) {
		//
		
	// システム変数
	} else if ( isSysvarNode(name) ) {
		int iSysvar = getSysvarIndex( &name[1] );
		
		if ( iSysvar >= 0 ) {
			return SysvarData[iSysvar].type;
		}
		
	// 静的変数
	} else {
		PVal *pval( seekSttVar( name ) );
		
		if ( pval ) return pval->flag;
	}
	return HSPVAR_FLAG_NONE;
}

//------------------------------------------------
// 変数情報のテキストを取得する
//------------------------------------------------
static CString VarTree_getItemVarText( HWND hwndTree, HTREEITEM hItem )
{
	const CString itemText = TreeView_GetItemString( hwndTree, hItem );
	const char*   name     = itemText.c_str();
	
	// ルート
	if ( isModuleNode(name) ) {
		auto varinf = new CVarinfoLine( *g_dbginfo, stt_maxlenVarinfo );
		
#ifdef with_WrapCall
		// 呼び出し履歴
		if ( strcmp(name, "+dynamic") == 0 ) {
			size_t const lenCall = g_stkCallInfo.size();
			varinf->cat( "[呼び出し履歴]" );
			varinf->cat_crlf();
			
			for ( uint i = 0; i < lenCall; ++ i ) {
				auto const pCallInfo = g_stkCallInfo[i];
				varinf->addCall( pCallInfo->pStDat, ModcmdCallInfo_getPrmstk(*pCallInfo) );
				varinf->cat_crlf();
			}
			
			// 最後の返値
			if ( stt_bResultNode && g_pLastResult != nullptr ) {
				CString result = "-> " + g_pLastResult->valueString;
				varinf->cat( result.c_str() );
			}
		} else
#endif
		// システム変数
		if ( strcmp(name, "+sysvar") == 0 ) {
			varinf->cat( "[システム変数]" );
			varinf->cat_crlf();
			
			for ( int i = 0; i < SysvarCount; ++ i ) {
				varinf->catf("%s\t= ", SysvarData[i].name );
				varinf->addSysvar( i );
				varinf->cat_crlf();
			}
			
		// モジュール (@...)
		} else {
			varinf->catf( "[%s]", name );
			varinf->cat_crlf();
			
			for ( HTREEITEM hElem = TreeView_GetNextItem( hwndTree, hItem, TVGN_CHILD )
				; hElem != NULL
				; hElem  = TreeView_GetNextSibling( hwndTree, hElem )
			) {
				CString&& _nodetext = TreeView_GetItemString( hwndTree, hElem );
				const char* const nodetext = _nodetext.c_str();
				
				if ( isModuleNode(nodetext) ) {		// 入れ子モジュール
					varinf->cat( nodetext );		// 名前を出力するだけ
				} else {
					auto&& varname_specific = _nodetext.substr( 0, _nodetext.find('@') );	// 最初の '@' 以下を切り捨てる
					varinf->catf( "%s\t= ", varname_specific.c_str() );
					varinf->addVar( seekSttVar(nodetext) );
				}
				varinf->cat_crlf();
			}
		}	// 他 => 無視
		
		CString result = varinf->getString();		// 返却用にコピー
		delete varinf;
		
		return result;
		
	// リーフ
	} else {
	/*
		// HSP側に問い合わせ
		char *p = g_debug->get_varinf( name, GetTabVarsOption() );
		
		SetWindowText( g_hVarEdit, p );
		
		g_debug->dbg_close( p );
	/*/
		
		CVarinfoText* varinf = new CVarinfoText( *g_dbginfo, stt_maxlenVarinfo );
		
		// システム変数
		if ( isSysvarNode(name) ) {
			varinf->addSysvar( &name[1] );
			
	#ifdef with_WrapCall
		// 呼び出しの情報
		} else if ( isCallNode(name) ) {
			auto const pCallInfo = almighty_cast<const ModcmdCallInfo*>(
				TreeView_GetItemLParam( g_hVarTree, hItem )
			);
			
			if ( pCallInfo != NULL ) {
				varinf->addCall( pCallInfo->pStDat, ModcmdCallInfo_getPrmstk(*pCallInfo), pCallInfo->sublev, &name[1] );
			}
		// 返値データ
		} else if ( stt_bResultNode && isResultNode(name) ) {
			auto const pResult = almighty_cast<const ResultNodeData*>(
				TreeView_GetItemLParam( g_hVarTree, hItem )
			);
			varinf->addResult2( pResult->valueString, STRUCTDAT_getName(pResult->pStDat) );
	#endif
		// 静的変数
		} else {
			PVal *pval = seekSttVar( name );
			
			if ( pval == NULL ) {
				return strf("[Error] \"%s\"は静的変数の名称ではない。\n参照：静的変数が存在しないときにこのエラーが生じることがある。", name);
			}
			
			varinf->addVar( pval, name );
		}
		
		CString result = varinf->getString();	// 返却用にコピー
		
		delete varinf;
		return result;
	}
}

//##############################################################################
//                WrapCall 関連
//##############################################################################
#ifdef with_WrapCall

static size_t g_cntWillAddCallNodes = 0;	// 次の更新で追加すべきノード数

static std::vector<ResultNodeData*> g_willAddResultNodes;		// 次の更新で追加すべき返値ノード
static ResultNodeData* g_willAddResultNodeIndepend = nullptr;	// 〃 ( +dynamic 直下 )

//------------------------------------------------
// Dynamic 関連のデータをすべて破棄する
//------------------------------------------------
void termNodeDynamic()
{
	if ( stt_bResultNode ) {
		delete g_willAddResultNodeIndepend; g_willAddResultNodeIndepend = nullptr;
		for each ( auto it in g_willAddResultNodes ) delete it;
		g_willAddResultNodes.clear();
		
		CallTree_RemoveDependResult( g_hVarTree, g_hNodeDynamic );
	}
	return;
}

//------------------------------------------------
// ResultNodeData の生成
// 
// @ OnEndCaling でしか呼ばれない。
//------------------------------------------------
ResultNodeData* NewResultNodeData( const ModcmdCallInfo& callinfo, void* ptr, int flag )
{
	auto pResult = new ResultNodeData;
		pResult->pStDat      = callinfo.pStDat;
		pResult->sublev      = callinfo.sublev;
		pResult->valueString = "";
		pResult->pCallInfoDepended = callinfo.prev;
	
	{
		auto const varinf = new CVarinfoLine( *g_dbginfo, stt_maxlenVarinfo );
		varinf->addResult( ptr, flag );
		
		pResult->valueString = varinf->getString();
		
		delete varinf;
	}
	return pResult;		// delete 義務
}

ResultNodeData* NewResultNodeData( const ModcmdCallInfo& callinfo, PVal* pvResult )
{
	return NewResultNodeData( callinfo, pvResult->pt, pvResult->flag );
}

//------------------------------------------------
// 依存していた返値ノードをすべて削除する
// 
// @ 終了時に hItem = g_hNodeDynamic で呼ばれるかも。
//------------------------------------------------
static void CallTree_RemoveDependResult( HWND hwndTree, HTREEITEM hItem )
{
	for ( HTREEITEM hChild = TreeView_GetChild( hwndTree, hItem )
		; hChild != NULL
		;
	) {
		HTREEITEM hNext = TreeView_GetNextSibling( hwndTree, hChild );
		CString&& nodeName = TreeView_GetItemString(hwndTree, hChild);
		if ( isResultNode(nodeName.c_str()) ) {
			RemoveResultNode( hwndTree, hChild );
		}
		hChild = hNext;
	}
	return;
}

//------------------------------------------------
// prmstack を参照する
//------------------------------------------------
static void* ModcmdCallInfo_getPrmstk(const ModcmdCallInfo& callinfo)
{
	if ( callinfo.next == nullptr ) { return ctx->prmstack; }			// 最新の呼び出し
	if ( callinfo.isRunning() ) { return callinfo.next->prmstk_bak; }	// 実行中の呼び出し (引数展開が終了している)
	return nullptr;		// 引数展開中 (スタックフレームが未完成なので prmstack は参照できない)
}

//------------------------------------------------
// 呼び出し開始
//------------------------------------------------
void OnBgnCalling( HWND hwndTree, const ModcmdCallInfo& callinfo )
{
	g_stkCallInfo.push_back( &callinfo );
	
	// ノードの追加
	if ( !stt_bStepRunning ) {
		g_cntWillAddCallNodes ++;		// 後で追加する
	} else {
		AddCallNode( hwndTree, callinfo );
	}
	
	// ログ出力
	if ( IsDlgButtonChecked( g_hLogPage, IDC_CHK_CALOG ) ) {
		CString logText = strf(
			"[CallBgn] %s\t@%d of \"%s\"]\n",
			STRUCTDAT_getName(callinfo.pStDat),
			callinfo.line,
			callinfo.fname
		);
		TabLogAdd( logText.c_str() );
	}
	
	ctx->retval_level = -1;
	
	return;
}

//------------------------------------------------
// 呼び出し終了
//------------------------------------------------
void OnEndCalling( HWND hwndTree, const ModcmdCallInfo& callinfo )
{
	if ( g_stkCallInfo.empty() ) return;
	
	auto const pResult = stt_bResultNode && ( ctx->retval_level == ctx->sublev + 1 )
		? NewResultNodeData( callinfo, *exinfo->mpval )
		: nullptr;
	
	// ログ出力
	if ( IsDlgButtonChecked( g_hLogPage, IDC_CHK_CALOG ) ) {
		CString logText = strf(
			"[CallEnd] %s%s\n",
			STRUCTDAT_getName(callinfo.pStDat),
			(pResult ? (" -> " + pResult->valueString).c_str() : "")
		);
		TabLogAdd( logText.c_str() );
	}
	
	// ノードを削除
	if ( g_cntWillAddCallNodes > 0 ) {
		g_cntWillAddCallNodes --;				// やっぱり追加しない
	} else {
		RemoveCallNode( hwndTree, callinfo );	// 既に追加していたので除去される
	}
	
	// 返値ノードの追加
	if ( pResult ) {
		if ( !stt_bStepRunning ) {	// 後で追加する
			if ( pResult->pCallInfoDepended ) {
				g_willAddResultNodes.push_back(pResult);
			} else {
				delete g_willAddResultNodeIndepend;
				g_willAddResultNodeIndepend = pResult;
			}
		} else {
			AddResultNode( hwndTree, pResult );
		}
	}
	
	g_stkCallInfo.pop_back();
	return;
}

//------------------------------------------------
// 返値返却
// 
// @ 返値があれば、OnEndCalling の直前に呼ばれる。
// @ ptr, flag はすぐに死んでしまうので、
// @	今のうちに文字列化しておく。
//------------------------------------------------
void OnResultReturning( HWND hwndTree, const ModcmdCallInfo& callinfo, void* ptr, int flag )
{
	return;
}

//------------------------------------------------
// 呼び出しノードを追加
//------------------------------------------------
void AddCallNode( HWND hwndTree, const ModcmdCallInfo& callinfo )
{
	char name[128] = "'";
	strcpy_s( &name[1], sizeof(name) - 1, STRUCTDAT_getName(callinfo.pStDat) );
	
	TVINSERTSTRUCT tvis = { 0 };
	tvis.hParent      = g_hNodeDynamic;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask    = TVIF_TEXT | TVIF_PARAM;
	tvis.item.pszText = name;
	tvis.item.lParam  = almighty_cast<LPARAM>( &callinfo );		// lparam に ModcmdCallInfo を設定する
	
	HTREEITEM hChild = TreeView_InsertItem( hwndTree, &tvis );
	
	// 第一ノードなら自動的に開く
	if ( TreeView_GetChild( g_hVarTree, g_hNodeDynamic ) == hChild ) {
		TreeView_Expand( g_hVarTree, g_hNodeDynamic, TVE_EXPAND );
	}
	return;
}

//------------------------------------------------
// (最後の) 呼び出しノードを削除
//------------------------------------------------
void RemoveCallNode( HWND hwndTree, const ModcmdCallInfo& callinfo )
{
	HTREEITEM hLast = TreeView_GetChild( hwndTree, g_hNodeDynamic );
	if ( hLast == NULL ) return;		// error
	
	// 末子を取得
	for ( HTREEITEM hNext = hLast
		; hNext != NULL
		; hNext = TreeView_GetNextSibling( hwndTree, hLast )
	) {
		hLast = hNext;
	}
	
	// フォーカスを回避
	TreeView_EscapeFocus( hwndTree, hLast );
	
	// 削除
	if ( stt_bResultNode ) {
		CallTree_RemoveDependResult( hwndTree, hLast );	// 依存していた返値ノードを除去する
	}
	TreeView_DeleteItem( hwndTree, hLast );
	return;
}

//------------------------------------------------
// 呼び出しノード更新
//------------------------------------------------
void UpdateCallNode( HWND hwndTree )
{
	// 追加予定の返値ノードを実際に追加する
	if ( stt_bResultNode && g_willAddResultNodeIndepend ) {
		AddResultNode( hwndTree, g_willAddResultNodeIndepend );
		g_willAddResultNodeIndepend = nullptr;
	}
	
	// 追加予定の呼び出しノードを実際に追加する
	if ( g_cntWillAddCallNodes > 0 ) {
		size_t const lenStk = g_stkCallInfo.size() ;
		for ( size_t i = lenStk - g_cntWillAddCallNodes; i < lenStk; ++ i ) {
			AddCallNode( hwndTree, *g_stkCallInfo[i] );
		}
		g_cntWillAddCallNodes = 0;
	}
	
	// 追加予定の返値ノードを実際に追加する (2)
	if ( stt_bResultNode && !g_willAddResultNodes.empty() ) {
		for each ( auto pResult in g_willAddResultNodes ) {
			AddResultNode( hwndTree, pResult );
		}
		g_willAddResultNodes.clear();
	}
	return;
}

//------------------------------------------------
// 返値ノードを追加
//------------------------------------------------
void AddResultNode( HWND hwndTree, const ResultNodeData* pResult )
{
	HTREEITEM hElem;
	if ( pResult->pCallInfoDepended ) {
		for ( hElem = TreeView_GetChild( hwndTree, g_hNodeDynamic )
			; hElem != NULL
			; hElem = TreeView_GetNextSibling( hwndTree, hElem )
		) {
			auto const pCallInfo = almighty_cast<const ModcmdCallInfo*>(TreeView_GetItemLParam(hwndTree, hElem));
			if ( pCallInfo == pResult->pCallInfoDepended ) break;
		}
		if ( hElem == NULL ) {		// 依存元がなければ追加しない
			delete pResult;
			return;
		}
		
		if ( pResult->pCallInfoDepended->isRunning() ) {
			CallTree_RemoveDependResult( hwndTree, hElem );
		}
	} else {
		CallTree_RemoveDependResult( hwndTree, g_hNodeDynamic );
		hElem = g_hNodeDynamic;
	}
	
	char name[128] = "\"";
	strcpy_s( &name[1], sizeof(name) - 1, STRUCTDAT_getName(pResult->pStDat) );
	
	TVINSERTSTRUCT tvis = { 0 };
		tvis.hParent      = hElem;	// 依存元を親にする
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask    = TVIF_TEXT | TVIF_PARAM;
		tvis.item.pszText = name;
		tvis.item.lParam  = almighty_cast<LPARAM>( pResult );
	
	// 挿入
	HTREEITEM hChild = TreeView_InsertItem( hwndTree, &tvis );
	
	// 第一ノードなら自動的に開く
	if ( TreeView_GetChild( g_hVarTree, hElem ) == hChild ) {
		TreeView_Expand( g_hVarTree, hElem, TVE_EXPAND );
	}
	
	g_pLastResult = pResult;		// 最後の返値を更新
	return;
}

//------------------------------------------------
// 返値ノードを削除
//------------------------------------------------
void RemoveResultNode( HWND hwndTree, HTREEITEM hResult )
{
	// すべての子ノードを削除する
	CallTree_RemoveDependResult( hwndTree, hResult );
	
	// フォーカスを回避
	TreeView_EscapeFocus( hwndTree, hResult );
	
	// 関連していた返値ノードデータを破棄
	auto pResult = almighty_cast<ResultNodeData*>( TreeView_GetItemLParam( hwndTree, hResult ) );
	if ( g_pLastResult == pResult ) g_pLastResult = nullptr;
	delete pResult;
	
	// 削除
	TreeView_DeleteItem( hwndTree, hResult );
	
	return;
}

//------------------------------------------------
// WrapCall メソッド
//------------------------------------------------
static void WrapCallMethod_AddLog( const char* log )
{
	TabLogAdd( log );
	TabLogAddCrlf();
}

static void WrapCallMethod_BgnCalling( unsigned int idx, const ModcmdCallInfo* pCallInfo )
{
	OnBgnCalling( g_hVarTree, *pCallInfo );
}

static int WrapCallMethod_EndCalling( unsigned int idx, const ModcmdCallInfo* pCallInfo )
{
	OnEndCalling( g_hVarTree, *pCallInfo );
	return RUNMODE_RUN;
}

static void WrapCallMethod_ResultReturning( unsigned int idx, const ModcmdCallInfo* pCallInfo, void* ptr, int flag )
{
//	OnResultReturning( g_hVarTree, *pCallInfo, ptr, flag );
}

#endif
