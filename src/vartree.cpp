
#include <Windows.h>
#include <CommCtrl.h>

#include "main.h"
#include "dialog.h"
#include "config_mng.h"

#include "vartree.h"
#include "CVarinfoText.h"
#include "CVarinfoTree.h"
#include "CVarinfoLine.h"

#include "SysvarData.h"
#include "ClhspDebugInfo.h"

extern CVarTree* getSttVarTree();	// at main.cpp

namespace VarTree
{

static vartype_t getVartype( const char *name );
static inline PVal* seekSttVar( const char *name )
{
	const int iVar = g_dbginfo->exinfo->HspFunc_seekvar( name );
	return ( iVar >= 0 ) ? &g_dbginfo->ctx->mem_var[iVar] : nullptr;
}

HTREEITEM g_hNodeDynamic;

#ifdef with_WrapCall
static const ResultNodeData* g_pLastResult = nullptr;
#endif

//------------------------------------------------
// 変数ツリーの初期化
//------------------------------------------------
void init( HWND hVarTree )
{
	TreeView_DeleteAllItems( hVarTree );
	
	// 静的変数リストをツリーに追加する
	CVarTree* pVarTree = getSttVarTree();
	
	addNode       ( hVarTree, TVI_ROOT, *pVarTree );
#ifdef with_WrapCall
	addNodeDynamic( hVarTree, TVI_ROOT );
#endif
	addNodeSysvar ( hVarTree, TVI_ROOT );
	
	// すべてのルートノードを開く
	HTREEITEM hRoot( TreeView_GetRoot( hVarTree ) );
	
	for ( HTREEITEM hNode = hRoot
		; hNode != NULL
		; hNode = TreeView_GetNextSibling( hVarTree, hNode )
	) {
		TreeView_Expand( hVarTree, hNode, TVE_EXPAND );
	}
	
	// トップを表示するように仕向ける
	TreeView_EnsureVisible( hVarTree, hRoot );
	
	return;
}

//------------------------------------------------
// 変数ツリー終了時
//------------------------------------------------
void term()
{
#ifdef with_WrapCall
	CallTree_RemoveDependResult( Dialog::getVarTreeHandle(), g_hNodeDynamic );
#endif
}

//------------------------------------------------
// 変数ツリーに要素を追加する
//------------------------------------------------
void addNode( HWND hwndTree, HTREEITEM hParent, CVarTree& tree )
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
				addNode( hwndTree, hParent, *iter );
			}
			
			break;
		}
	}
	
	return;
}

//------------------------------------------------
// 変数ツリーにシステム変数ノードを追加する
//------------------------------------------------
void addNodeSysvar( HWND hwndTree, HTREEITEM hParent )
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
void addNodeDynamic( HWND hwndTree, HTREEITEM hParent )
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
LRESULT customDraw( HWND hwndTree, LPNMTVCUSTOMDRAW pnmcd )
{
	if ( pnmcd->nmcd.dwDrawStage == CDDS_PREPAINT ) {
		return CDRF_NOTIFYITEMDRAW;
		
	} else if ( pnmcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT ) {
		
		HTREEITEM hItem = reinterpret_cast<HTREEITEM>( pnmcd->nmcd.dwItemSpec );//( TreeView_GetSelection(hwndTree) );
		string    sItem = TreeView_GetItemString( hwndTree, hItem );
		const char *name ( sItem.c_str() );
		
		if ( isModuleNode(name) ) {
		//	pnmcd->clrText   = g_config->clrTypeText[HSPVAR_FLAG_NONE];
		//	pnmcd->clrText   = RGB(   0,   0,   0 );
		//	pnmcd->clrTextBk = RGB( 255, 255, 255 );
			return 0;
			
		} else /* if ( isSysvarNode(name) or isSttVarNode(name) ) */ {
			vartype_t vtype ( getVartype(name) );
			
			// 組み込み型
			if ( 0 < vtype && vtype < HSPVAR_FLAG_USERDEF ) {
				pnmcd->clrText   = g_config->clrTypeText[vtype];
				
			// 拡張型
			} else {
				pnmcd->clrText   = g_config->clrTypeText[0];
			}
		}
		return CDRF_NEWFONT;
	}
	
	return 0;
}

//------------------------------------------------
// 変数の型を取得する
//------------------------------------------------
vartype_t getVartype( const char *name )
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

	// 呼び出し | 返値
#ifdef with_WrapCall
	} else if ( isCallNode(name) || isResultNode(name) ) {
		//
#endif
		
	// 静的変数
	} else {
		PVal *pval( seekSttVar( name ) );
		
		if ( pval ) return pval->flag;
	}
	return HSPVAR_FLAG_NONE;
}

//------------------------------------------------
// 変数ツリーの要素の型を取得する
//------------------------------------------------
vartype_t getVartype( HWND hwndTree, HTREEITEM hItem )
{
	CString    sItem( TreeView_GetItemString( hwndTree, hItem ) );
	const char *name( sItem.c_str() );
	
	return getVartype( name );
}

//------------------------------------------------
// 変数情報のテキストを取得する
//------------------------------------------------
CString getItemVarText( HWND hwndTree, HTREEITEM hItem )
{
	const CString itemText = TreeView_GetItemString( hwndTree, hItem );
	const char*   name     = itemText.c_str();
	
	// ノード
	if ( isModuleNode(name) ) {
		auto varinf = new CVarinfoLine( *g_dbginfo, g_config->maxlenVarinfo );
		
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
			if ( g_config->bResultNode && g_pLastResult != nullptr ) {
				auto const result = "-> " + g_pLastResult->valueString;
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
		
		SetWindowText( g_dialog.hVarEdit, p );
		
		g_debug->dbg_close( p );
	/*/
		
		CVarinfoText* varinf = new CVarinfoText( *g_dbginfo, g_config->maxlenVarinfo );
		
		// システム変数
		if ( isSysvarNode(name) ) {
			varinf->addSysvar( &name[1] );
			
	#ifdef with_WrapCall
		// 呼び出しの情報
		} else if ( isCallNode(name) ) {
			auto const pCallInfo = reinterpret_cast<const ModcmdCallInfo*>(
				TreeView_GetItemLParam( hwndTree, hItem )
			);
			
			if ( pCallInfo != NULL ) {
				varinf->addCall( pCallInfo->pStDat, ModcmdCallInfo_getPrmstk(*pCallInfo), pCallInfo->sublev, &name[1], pCallInfo->fname, pCallInfo->line );
			}
			
		// 返値データ
		} else if ( g_config->bResultNode && isResultNode(name) ) {
			auto const pResult = reinterpret_cast<const ResultNodeData*>(
				TreeView_GetItemLParam( hwndTree, hItem )
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

#ifdef with_WrapCall

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
	tvis.item.lParam  = (LPARAM)( &callinfo );		// lparam に ModcmdCallInfo を設定する
	
	HTREEITEM hChild = TreeView_InsertItem( hwndTree, &tvis );
	
	// 第一ノードなら自動的に開く
	if ( TreeView_GetChild( hwndTree, g_hNodeDynamic ) == hChild ) {
		TreeView_Expand( hwndTree, g_hNodeDynamic, TVE_EXPAND );
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
	if ( g_config->bResultNode ) {
		CallTree_RemoveDependResult( hwndTree, hLast );	// 依存していた返値ノードを除去する
	}
	TreeView_DeleteItem( hwndTree, hLast );
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
			auto const pCallInfo = reinterpret_cast<const ModcmdCallInfo*>(TreeView_GetItemLParam(hwndTree, hElem));
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
		tvis.item.lParam  = (LPARAM)( pResult );
	
	// 挿入
	HTREEITEM hChild = TreeView_InsertItem( hwndTree, &tvis );
	
	// 第一ノードなら自動的に開く
	if ( TreeView_GetChild( hwndTree, hElem ) == hChild ) {
		TreeView_Expand( hwndTree, hElem, TVE_EXPAND );
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
	auto pResult = reinterpret_cast<ResultNodeData*>( TreeView_GetItemLParam( hwndTree, hResult ) );
	if ( g_pLastResult == pResult ) g_pLastResult = nullptr;
	delete pResult;
	
	// 削除
	TreeView_DeleteItem( hwndTree, hResult );
	
	return;
}

#endif

} // namespace VarTree
