
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
#include "DebugInfo.h"

extern CVarTree* getSttVarTree();	// at main.cpp

namespace VarTree
{
static vartype_t getVartypeFromNodeName(char const* name);

HTREEITEM g_hNodeDynamic;

#ifdef with_WrapCall
static void AddCallNodeImpl(HWND hwndTree, ModcmdCallInfo const& callinfo);
static void RemoveCallNodeImpl(HWND hwndTree, ModcmdCallInfo const& callinfo);
static void AddResultNode(HWND hwndTree, ResultNodeData* pResult);

// 最後の返値ノード
static ResultNodeData const* g_pLastResult = nullptr;

// 動的ノードの追加除去の遅延の管理
static size_t g_cntWillAddCallNodes = 0;						// 次の更新で追加すべきノード数
static std::vector<ResultNodeData*> g_willAddResultNodes;		// 次の更新で追加すべき返値ノード
static ResultNodeData* g_willAddResultNodeIndepend = nullptr;	// 〃 ( +dynamic 直下 )
#endif

//------------------------------------------------
// 変数ツリーの初期化
//------------------------------------------------
void init( HWND hVarTree )
{
//	TreeView_DeleteAllItems( hVarTree );
	
	addNode( hVarTree, TVI_ROOT, *getSttVarTree() );
#ifdef with_WrapCall
	addNodeDynamic( hVarTree, TVI_ROOT );
#endif
	addNodeSysvar( hVarTree, TVI_ROOT );
	
	// すべてのルートノードを開く
	HTREEITEM const hRoot = TreeView_GetRoot( hVarTree );
	
	for ( HTREEITEM hNode = hRoot
		; hNode != nullptr
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
	if ( auto hVarTree = Dialog::getVarTreeHandle() ) {
		CallTree_RemoveDependResult(hVarTree, g_hNodeDynamic);

		// dynamic 関連のデータを削除する
		if ( utilizeResultNodes() ) {
			delete g_willAddResultNodeIndepend; g_willAddResultNodeIndepend = nullptr;
			for each (auto it in g_willAddResultNodes) delete it;
			g_willAddResultNodes.clear();
		}
	}
#endif
}

//------------------------------------------------
// 変数ツリーにノードを追加する
//------------------------------------------------
void addNode(HWND hwndTree, HTREEITEM hParent, CVarTree const& tree)
{
	TVINSERTSTRUCT tvis;
	tvis.hParent      = hParent;
	tvis.hInsertAfter = TVI_SORT;
	tvis.item.mask    = TVIF_TEXT | TVIF_PARAM;
	tvis.item.pszText = const_cast<char*>( tree.getName().c_str() );
	// 静的変数やモジュールの lParam 値は、CVarTree の対応するノードへのポインタ
	tvis.item.lParam  = (LPARAM)&tree;

	auto const hElem = TreeView_InsertItem( hwndTree, &tvis );
	if ( auto const modnode = tree.asCaseOf<CStaticVarTree::ModuleNode>() ) {
		for ( auto const& iter : *modnode ) {
			//for ( CVarTree::const_iterator iter = tree.begin(); iter != tree.end(); ++iter ) {
			addNode(hwndTree, hElem, *iter.second);
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
	
	HTREEITEM const hNodeSysvar = TreeView_InsertItem( hwndTree, &tvis );
	
	tvis.hParent      = hNodeSysvar;
	tvis.hInsertAfter = TVI_LAST;		// 順番を守る
	
	// システム変数のリストを追加する
	for ( int i = 0; i < SysvarCount; ++ i ) {
		string const name = strf( "~%s", SysvarData[i].name );
		tvis.item.pszText = const_cast<char*>( name.c_str() );
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
		auto const hItem = reinterpret_cast<HTREEITEM>(pnmcd->nmcd.dwItemSpec);//( TreeView_GetSelection(hwndTree) );
		string const sItem = TreeView_GetItemString( hwndTree, hItem );
		char const* const name = sItem.c_str();
		
		if ( isModuleNode(name) ) {
		//	pnmcd->clrText   = g_config->clrTypeText[HSPVAR_FLAG_NONE];
		//	pnmcd->clrText   = RGB(   0,   0,   0 );
		//	pnmcd->clrTextBk = RGB( 255, 255, 255 );
			return 0;
			
		} else /* if ( isSysvarNode(name) || isSttVarNode(name) ) */ {
			vartype_t const vtype = getVartypeFromNodeName(name);
			
			// 組み込み型
			if ( 0 < vtype && vtype < HSPVAR_FLAG_USERDEF ) {
				pnmcd->clrText = g_config->clrTypeText[vtype];
				
			// 拡張型
			} else {
				pnmcd->clrText = g_config->clrTypeText[0];
			}
		}
		return CDRF_NEWFONT;
	}
	
	return 0;
}

//------------------------------------------------
// 変数の型を取得する
//------------------------------------------------
vartype_t getVartypeFromNodeName( char const* name )
{
	// モジュールノード
	if ( isModuleNode(name) ) {
		//
		
	// システム変数
	} else if ( isSysvarNode(name) ) {
		int const iSysvar = Sysvar_seek( &name[1] );
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
		PVal* const pval = hpimod::seekSttVar( name );
		if ( pval ) return pval->flag;
	}
	return HSPVAR_FLAG_NONE;
}

//------------------------------------------------
// 変数ツリーの要素の型を取得する
//------------------------------------------------
vartype_t getVartype( HWND hwndTree, HTREEITEM hItem )
{
	return getVartypeFromNodeName(TreeView_GetItemString(hwndTree, hItem).c_str());
}

//------------------------------------------------
// 変数情報のテキストを取得する
//------------------------------------------------
string getItemVarText( HWND hwndTree, HTREEITEM hItem )
{
	string const itemText = TreeView_GetItemString( hwndTree, hItem );
	char const* const name = itemText.c_str();
	
	// ノード
	if ( isModuleNode(name) ) {
		auto const varinf = std::make_unique<CVarinfoLine>(g_config->maxlenVarinfo);
		
#ifdef with_WrapCall
		if ( strcmp(name, "+dynamic") == 0 ) {
			varinf->addCallsOverview( (utilizeResultNodes() && g_pLastResult ? g_pLastResult : nullptr) );
		} else
#endif
		if ( strcmp(name, "+sysvar") == 0 ) {
			varinf->addSysvarsOverview();

		// モジュール (@...)
		} else {
			auto const* pTree = reinterpret_cast<CVarTree::ModuleNode*>(
				TreeView_GetItemLParam(hwndTree, hItem)
			);
			assert(pTree != nullptr);
			varinf->addModuleOverview(name, *pTree);
		}
		string const result = varinf->getString();		// 返却用にコピー
		return std::move(result);
		
	// リーフ
	} else {
		auto const varinf = std::make_unique<CVarinfoText>(g_config->maxlenVarinfo);
		
		if ( isSysvarNode(name) ) {
			varinf->addSysvar( &name[1] );
			
	#ifdef with_WrapCall
		} else if ( isCallNode(name) ) {
			auto const idx = static_cast<int>(
				TreeView_GetItemLParam( hwndTree, hItem )
			);
			assert(idx >= 0);
			if ( auto const pCallInfo = WrapCall::getCallInfoAt(idx) ) {
				varinf->addCall(*pCallInfo);
			}
			
		// 返値データ
		} else if ( utilizeResultNodes() && isResultNode(name) ) {
			auto const pResult = reinterpret_cast<ResultNodeData const*>(
				TreeView_GetItemLParam( hwndTree, hItem )
			);
			assert(pResult != nullptr);
			varinf->addResult2( pResult->valueString, hpimod::STRUCTDAT_getName(pResult->stdat) );
	#endif
		// 静的変数
		} else {
		/*
			// HSP側に問い合わせ
			char* p = g_debug->get_varinf( name, GetTabVarsOption() );
			SetWindowText( g_dialog.hVarEdit, p );
			g_debug->dbg_close( p );
		//*/
			PVal* const pval = hpimod::seekSttVar( name );
			if ( !pval ) {
				return strf("[Error] \"%s\"は静的変数の名称ではない。\n参照：静的変数が存在しないときにこのエラーが生じることがある。", name);
			}
			varinf->addVar( pval, name );
		}
		
		string const result = varinf->getString();	// 返却用にコピー
		return std::move(result);
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
	if ( !utilizeResultNodes() ) return;
	
	for ( HTREEITEM hChild = TreeView_GetChild( hwndTree, hItem )
		; hChild != nullptr
		;
	) {
		HTREEITEM const hNext = TreeView_GetNextSibling( hwndTree, hChild );
		string const nodeName = TreeView_GetItemString(hwndTree, hChild);
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
void AddCallNode(HWND hwndTree, ModcmdCallInfo const& callinfo)
{
	if ( !Knowbug::isStepRunning() ) {
		// 次に停止したときにまとめて追加する
		++g_cntWillAddCallNodes;
	} else {
		AddCallNodeImpl(hwndTree, callinfo);
	}
	return;
}

void AddCallNodeImpl( HWND hwndTree, ModcmdCallInfo const& callinfo )
{
	char name[128] = "'";
	strcpy_s( &name[1], sizeof(name) - 1, hpimod::STRUCTDAT_getName(callinfo.stdat) );
	
	TVINSERTSTRUCT tvis = { 0 };
	tvis.hParent      = g_hNodeDynamic;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask    = TVIF_TEXT | TVIF_PARAM;
	tvis.item.pszText = name;
	tvis.item.lParam  = (LPARAM)(callinfo.idx);		// lparam に ModcmdCallInfo の添字を設定する
	
	HTREEITEM const hChild = TreeView_InsertItem( hwndTree, &tvis );
	
	// 第一ノードなら自動的に開く
	if ( TreeView_GetChild( hwndTree, g_hNodeDynamic ) == hChild ) {
		TreeView_Expand( hwndTree, g_hNodeDynamic, TVE_EXPAND );
	}
	return;
}

//------------------------------------------------
// (最後の) 呼び出しノードを削除
//------------------------------------------------
void RemoveCallNode(HWND hwndTree, ModcmdCallInfo const& callinfo)
{
	if ( g_cntWillAddCallNodes > 0 ) {
		--g_cntWillAddCallNodes;		// やっぱり追加しない
	} else {
		RemoveCallNodeImpl(hwndTree, callinfo);
	}
	return;
}

void RemoveCallNodeImpl(HWND hwndTree, ModcmdCallInfo const& callinfo)
{
	HTREEITEM const hLast = TreeView_GetChildLast( hwndTree, g_hNodeDynamic );
	if ( !hLast ) return;
	
	TreeView_EscapeFocus( hwndTree, hLast );
	CallTree_RemoveDependResult( hwndTree, hLast );
	TreeView_DeleteItem( hwndTree, hLast );
	return;
}

//------------------------------------------------
// 返値ノードを追加
/*
返値データ ptr の生存期間は今だけなので、今のうちに文字列化しなければいけない。
返値ノードも、呼び出しノードと同様に、次に実行が停止したときにまとめて追加する。

「A( B() )」のように、ユーザ定義命令・関数の引数の中でユーザ定義関数が呼ばれている場合、
「A は B に依存する」と表現することにする。A もユーザ定義関数である場合のみ考える。
このとき B の実行が終了してから A の実行が始まる。
B の返値ノードは、A の呼び出しノードの子ノードとして追加される。

表示がごちゃごちゃしないように、返値ノードは一定期間のうちに削除される。
*/
//------------------------------------------------
ResultNodeData const* AddResultNode(HWND hwndTree, ModcmdCallInfo const& callinfo, void* ptr, vartype_t flag)
{
	auto const pResult =
		(utilizeResultNodes() && ptr != nullptr && flag != HSPVAR_FLAG_NONE)
		? new ResultNodeData(callinfo, ptr, flag)
		: nullptr;

	if ( pResult ) {
		if ( !Knowbug::isStepRunning() ) {
			// 次に停止したときに追加する
			if ( pResult->pCallInfoDepended ) {
				g_willAddResultNodes.push_back(pResult);
			} else {
				delete g_willAddResultNodeIndepend;
				g_willAddResultNodeIndepend = pResult;
			}
		} else {
			AddResultNode(hwndTree, pResult);
		}
	}
	return pResult;
}

void AddResultNode(HWND hwndTree, ResultNodeData* pResult)
{
	// 依存する呼び出しのノードを検索する
	HTREEITEM hElem;
	if ( pResult->pCallInfoDepended ) {
		for ( hElem = TreeView_GetChild( hwndTree, g_hNodeDynamic )
			; hElem != nullptr
			; hElem = TreeView_GetNextSibling( hwndTree, hElem )
		) {
			int const idxCallInfo = static_cast<int>(TreeView_GetItemLParam(hwndTree, hElem));
			assert(idxCallInfo >= 0);
			if ( idxCallInfo == pResult->pCallInfoDepended->idx ) break;
		}

		// 依存元がなければ追加しない
		if ( hElem == nullptr ) {
			delete pResult;
			return;
		}
		
		if ( pResult->pCallInfoDepended->isRunning() ) {
			CallTree_RemoveDependResult( hwndTree, hElem );
		}

	// 依存しない返値の場合、+dynamic 自体に追加する
	} else {
		CallTree_RemoveDependResult( hwndTree, g_hNodeDynamic );
		hElem = g_hNodeDynamic;

		// 最後の返値を更新
		g_pLastResult = pResult;
	}
	
	// 挿入
	char name[128] = "\"";
	strcpy_s( &name[1], sizeof(name) - 1, hpimod::STRUCTDAT_getName(pResult->stdat) );
	
	TVINSERTSTRUCT tvis = { 0 };
		tvis.hParent      = hElem;	// 依存元を親にする
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask    = TVIF_TEXT | TVIF_PARAM;
		tvis.item.pszText = name;
		tvis.item.lParam  = (LPARAM)( pResult );
	
	HTREEITEM const hChild = TreeView_InsertItem( hwndTree, &tvis );
	
	// 第一ノードなら自動的に開く
	if ( TreeView_GetChild( hwndTree, hElem ) == hChild ) {
		TreeView_Expand( hwndTree, hElem, TVE_EXPAND );
	}
	return;
}

//------------------------------------------------
// 返値ノードを削除
//------------------------------------------------
void RemoveResultNode(HWND hwndTree, HTREEITEM hResult)
{
	CallTree_RemoveDependResult(hwndTree, hResult);

	TreeView_EscapeFocus(hwndTree, hResult);

	// 関連していた返値ノードデータを破棄
	{
		auto const pResult = reinterpret_cast<ResultNodeData*>(TreeView_GetItemLParam(hwndTree, hResult));
		if ( g_pLastResult == pResult ) g_pLastResult = nullptr;
		delete pResult;
	}

	TreeView_DeleteItem(hwndTree, hResult);
	return;
}

//------------------------------------------------
// 呼び出しノード更新
//------------------------------------------------
void UpdateCallNode(HWND hwndTree)
{
	// 追加予定の返値ノードを実際に追加する (part1)
	if ( utilizeResultNodes() && g_willAddResultNodeIndepend ) {
		AddResultNode(hwndTree, g_willAddResultNodeIndepend);
		g_willAddResultNodeIndepend = nullptr;
	}

	// 追加予定の呼び出しノードを実際に追加する
	if ( g_cntWillAddCallNodes > 0 ) {
		auto const range = WrapCall::getCallInfoRange();
		size_t const lenStk = std::distance(range.first, range.second);
		for ( size_t i = lenStk - g_cntWillAddCallNodes; i < lenStk; ++i ) {
			AddCallNodeImpl(hwndTree, *(range.first[i]));
		}
		g_cntWillAddCallNodes = 0;
	}

	// 追加予定の返値ノードを実際に追加する (part2)
	if ( utilizeResultNodes() && !g_willAddResultNodes.empty() ) {
		for (auto const pResult : g_willAddResultNodes) {
			AddResultNode(hwndTree, pResult);
		}
		g_willAddResultNodes.clear();
	}
	return;
}
#endif

} // namespace VarTree
