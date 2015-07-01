
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
static HWND g_hSttCtrl;

static HWND g_hLogPage;
static HWND g_hLogEdit;
static HWND g_hLogChkUpdate;

typedef BOOL (CALLBACK *HSP3DBGFUNC)(HSP3DEBUG *,int,int,int);

//----------------------------------------------------------

static HSPCTX*       ctx( NULL );
static HSPEXINFO* exinfo( NULL );

//----------------------------------------------------------

#include <vector>
#include "module/mod_cstring.h"
#include "module/mod_cast.h"
#include "ClhspDebugInfo.h"
#include "CVarTree.h"
//#include "CVarinfoTree.h"
#include "CVarinfoText.h"
#include "SysvarData.h"
#include "CIni.h"
#define dbgmsg(msg) MessageBox( NULL, msg, "Debug Window", MB_OK )

static DebugInfo  g_dbginfo_inst;
static DebugInfo* g_dbginfo      ( &g_dbginfo_inst );
static CVarTree*  stt_pSttVarTree( NULL );
//static DynTree_t* stt_pDynTree   ( NULL );
static CString*   stt_pLogmsg    ( NULL );
static std::vector<COLORREF> stt_clrTypeText;
static bool stt_bCustomDraw( false );
static int  stt_maxlenVarinfo( 0 );

static void getConfig( void );

static CVarTree* getSttVarTree( void );
static void VarTree_addNode( HWND hwndTree, HTREEITEM hParent, CVarTree& tree );
static void VarTree_addNodeSysvar( HWND hwndTree, HTREEITEM hParent );
static LRESULT   VarTree_customDraw( HWND hwndTree, LPNMTVCUSTOMDRAW pnmcd );
static vartype_t VarTree_getVartype( HWND hwndTree, HTREEITEM hItem );
static CString   VarTree_getItemString( HWND hwndTree, HTREEITEM hItem );

static PVal *seekSttVar( const char *name );
static vartype_t getVartype( const char *name );

static inline bool isModuleNode( const char *name ) { return name[0] == '@' || name[0] == '$'; }
static inline bool isSysvarNode( const char *name ) { return name[0] == '.'; }

#ifndef clhsp
EXPORT BOOL WINAPI debugbye( HSP3DEBUG *p1, int p2, int p3, int p4 );
#endif

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
		WS_CHILD | WS_VISIBLE,
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
			std::sprintf( val, "(%d, %d, %d)",
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
	
	VarTree_addNode      ( g_hVarTree, TVI_ROOT, *pVarTree );
	VarTree_addNodeSysvar( g_hVarTree, TVI_ROOT );
	
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
	CString sItem( VarTree_getItemString( g_hVarTree, TreeView_GetSelection(g_hVarTree) ) );
	const char *name( sItem.c_str() );
	
	// モジュールの情報
	if ( name[0] == '@' || name[0] == '$' ) {
		return;
		
	// 変数の情報
	} else {
	/*
		// HSP側に問い合わせ
		char *p = g_debug->get_varinf( name, GetTabVarsOption() );
		
		SetWindowText( g_hVarEdit, p );
		
		g_debug->dbg_close( p );
	/*/
		CVarinfoText *varinf( new CVarinfoText( *g_dbginfo, stt_maxlenVarinfo ) );
		
		// システム変数
		if ( name[0] == '.' ) {
			varinf->addSysvar( &name[1] );
			
		// 静的変数
		} else {
			PVal *pval = seekSttVar( name );
			
			if ( pval == NULL ) {
				dbgmsg( strf("[Error] \"%s\"は静的変数の名称ではない。", name).c_str() );
				return;
			}
			
			varinf->addVar( pval, name );
		}
		
		SetWindowText( g_hVarEdit, varinf->getString().c_str() );
		
		delete varinf;
	//*/
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
		almighty_cast<WPARAM>( &caret[0] ),
		almighty_cast<LPARAM>( &caret[1] )
	);
	
	int size = Edit_GetTextLength( g_hLogEdit );
	Edit_SetSel( g_hLogEdit, size, size );		// 最後尾にキャレットを向ける
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
static void TabLogAdd( char *str )
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
			
			TabVarsReset();
			return TRUE;
			
		/*
		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
			}
			return FALSE;
		//*/
			
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
			
			CheckDlgButton( g_hLogPage, IDC_CHK_UPDATE, BST_CHECKED );
			return TRUE;
			
		case WM_COMMAND:
			switch ( LOWORD(wp) ) {
				
				case IDC_CHK_UPDATE:
					// チェックが付けられたとき
					if ( IsDlgButtonChecked( g_hLogPage, IDC_CHK_UPDATE ) ) {
						TabLogCommit();
					}
					break;
					
			//	case IDC_BTN_CLEAR:
			//		TabLogClear();
			//		break;
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
	TCITEM	tc;
	RECT	rt;
	LPPOINT pt = (LPPOINT) &rt;
	HFONT hf;

	switch ( msg ) {

	// 初期化
	case WM_CREATE:
		
		hf = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
		g_hTabCtrl = GenerateObj( hDlg, WC_TABCONTROL, "", DIALOG_X0, DIALOG_Y0, DIALOG_X1, DIALOG_Y2, IDU_TAB, hf );
		g_hSttCtrl = GenerateObj( hDlg, "static",    "",  DIALOG_X0 + 180, DIALOG_Y1 + 4, DIALOG_X1 - 180, 48, 0, hf );
		g_hBtn1    = GenerateObj( hDlg, "button", "実行", DIALOG_X0 +   8, DIALOG_Y1 + 4, 80, 24, ID_BTN1, hf );
		g_hBtn2    = GenerateObj( hDlg, "button", "次行", DIALOG_X0 +  88, DIALOG_Y1 + 4, 40, 24, ID_BTN2, hf );
		g_hBtn3    = GenerateObj( hDlg, "button", "停止", DIALOG_X0 + 128, DIALOG_Y1 + 4, 40, 24, ID_BTN3, hf );
		
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

	case WM_NOTIFY:
	{
		NMHDR *nm = (NMHDR *)lp;		// タブコントロールのシート切り替え通知
		int cur   = TabCtrl_GetCurSel(g_hTabCtrl);
		for ( int i = 0; i < TABDLGMAX; ++ i ) {
			if ( i == cur ) {
				ShowWindow( g_hTabSheet[i], SW_SHOW );
			} else {
				ShowWindow( g_hTabSheet[i], SW_HIDE );
			}
		}
		break;
	}
	case WM_COMMAND:
		switch ( LOWORD(wp) ) {
		case ID_BTN1:
			g_debug->dbg_set( HSPDEBUG_RUN );
			SetWindowText( g_hSttCtrl, "" );
			break;
		case ID_BTN2:
			g_debug->dbg_set( HSPDEBUG_STEPIN );
			SetWindowText( g_hSttCtrl, "" );
			break;
		case ID_BTN3:
			g_debug->dbg_set( HSPDEBUG_STOP );
			break;
		}
		break;
		
	case WM_CLOSE:
		return FALSE;
		
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hDlg, msg, wp, lp) ;
}

//##############################################################################
//        デバッグウィンドウ::clhspから呼ばれるもの
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
		WS_CAPTION | WS_OVERLAPPED | WS_BORDER | WS_VISIBLE,
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
	ShowWindow( hDlgWnd, SW_SHOW );
    UpdateWindow( hDlgWnd );
	
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
			strcat_s( ctx->stmp, 1024, "\r\n" );
			TabLogAdd( ctx->stmp );
			break;
	}
	return 0;
}

//------------------------------------------------
// debugbye ptr  (type1)
//------------------------------------------------
EXPORT BOOL WINAPI debugbye( HSP3DEBUG *p1, int p2, int p3, int p4 )
{
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
		
		_splitpath( ownpath, drive, dir, NULL, NULL );
		sprintf_s( ownpath, "%s%s", drive, dir );
	}
	
	CIni ini( strf("%sknowbug.ini", ownpath).c_str() );
	
	// カスタムドローするかどうか
	stt_bCustomDraw = ini.getInt( "ColorType", "bCustomDraw", FALSE ) != FALSE;
	
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
	tvis.item.pszText = "$sysvar";
	
	HTREEITEM hNodeSysvar = TreeView_InsertItem( hwndTree, &tvis );
	
	tvis.hParent      = hNodeSysvar;
	tvis.hInsertAfter = TVI_LAST;		// 順番を守る
	
	// システム変数のリストを追加する
	for ( int i = 0; i < SysvarCount; ++ i ) {
		CString name = strf( ".%s", SysvarData[i].name );
		
		tvis.item.pszText = const_cast<char *>( name.c_str() );
		
		TreeView_InsertItem( hwndTree, &tvis );
	}
	
	return;
}

//------------------------------------------------
// 変数ツリーの項目名を取得する
//------------------------------------------------
static CString VarTree_getItemString( HWND hwndTree, HTREEITEM hItem )
{
	char stmp[256];
	
	TVITEM ti;
	ti.hItem      = hItem;
	ti.mask       = TVIF_TEXT;
	ti.pszText    = &stmp[0];
	ti.cchTextMax = sizeof(stmp) - 1;
	
	if ( TreeView_GetItem( hwndTree, &ti ) ) {
		return stmp;
	} else {
		return "";
	}
}

//------------------------------------------------
// 変数ツリーの NM_CUSTOMDRAW を処理する
//------------------------------------------------
static LRESULT VarTree_customDraw( HWND hwndTree, LPNMTVCUSTOMDRAW pnmcd )
{
	if ( pnmcd->nmcd.dwDrawStage == CDDS_PREPAINT ) {
		return CDRF_NOTIFYITEMDRAW;
		
	} else if ( pnmcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT ) {
		
		HTREEITEM hItem ( almighty_cast<HTREEITEM>( pnmcd->nmcd.dwItemSpec ) );//( TreeView_GetSelection(hwndTree) );
		CString   sItem ( VarTree_getItemString( hwndTree, hItem ) );
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
	CString    sItem( VarTree_getItemString( hwndTree, hItem ) );
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
