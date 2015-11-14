// visitor - Treeview-Node 生成者

#include "Node.h"
#include "CTvCreator.h"

#include "module/ptr_cast.h"
#include "module/strf.h"

#include <stack>
#include <vector>
#include <cstring>
#include <CommCtrl.h>
#include <windows.h>

namespace DataTree
{

//**********************************************************
//    メンバ変数の実体
//**********************************************************
struct CTvCreator::Impl
{
public:
	HWND hTreeView;
	
	std::stack<HTREEITEM> stkParent;
	std::vector<string> modnamelist;
	
private:
	HTREEITEM hParent;
	
	TVINSERTSTRUCT tvis;
	
public:
	//--------------------------------------------
	// 構築
	//--------------------------------------------
	Impl( HWND _hTreeView )
		: hTreeView( _hTreeView )
	{
		modnamelist.reserve( 1 );
		pushParent( TVI_ROOT );		// 番兵
		pushParent( TVI_ROOT );
		return;
	}
	
	//--------------------------------------------
	// stkParent の操作
	//--------------------------------------------
	void pushParent( HTREEITEM hParent_ )
	{
		stkParent.push( hParent_ );
		hParent = hParent_;
		return;
	}
	
	void popParent()
	{
		stkParent.pop();
		hParent = stkParent.top();
		return;
	}
	
	//--------------------------------------------
	// tvis の取得
	// 
	// @ 再入不可能性に注意
	//--------------------------------------------
	TVINSERTSTRUCT& getTvIs()
	{
		initializeTvIs();
		return tvis;
	}
	
private:
	//--------------------------------------------
	// tvis の初期化
	//--------------------------------------------
	void initializeTvIs()
	{
		std::memset( &tvis, 0x00, sizeof(tvis) );
		tvis.hParent      = hParent;
		tvis.hInsertAfter = TVI_SORT;	// TODO: 呼び出しノードなどはソートしてはいけない
		tvis.item.mask    = TVIF_TEXT;
		return;
	}
};

//**********************************************************
//    構築と解体
//**********************************************************
//------------------------------------------------
// 構築
//------------------------------------------------
CTvCreator::CTvCreator( HWND hTreeView )
	: m( new Impl( hTreeView ) )
{ }

//------------------------------------------------
// アイテムの挿入
//------------------------------------------------
void CTvCreator::insertItem(tree_t node, string name, HTREEITEM hInsertAfter)
{
	TVINSERTSTRUCT& tvis = m->getTvIs();
	tvis.hInsertAfter = hInsertAfter;
	tvis.item.mask |= TVIF_PARAM;
	tvis.item.lParam = ctype_cast<LPARAM>(address_cast(node));
	tvis.item.pszText = const_cast<char*>(name.c_str());

	m->pushParent(TreeView_InsertItem(m->hTreeView, &tvis));
	return;
}

//**********************************************************
//    インターフェース
//**********************************************************
//------------------------------------------------
// 訪問
//------------------------------------------------
void CTvCreator::visit( tree_t node )
{
	node->acceptVisitor(*this);
	return;
}

//**********************************************************
//    ノードの処理 (callback)
//**********************************************************
//++++++++++++++++++++++++++++++++++++++++++++++++
//    コンテナ
//++++++++++++++++++++++++++++++++++++++++++++++++
//------------------------------------------------
// 行き
//------------------------------------------------
void CTvCreator::procPre( IPolyNode* node )
{
	string name(node->getName());
	
	// 変数 => スコープ解決を付加する
	if (   typeid(node) == typeid(CNodeVarArray*)
		|| typeid(node) == typeid(CNodeVarElem*)
	) {
		for each ( string modname in m->modnamelist ) {
			name.append( "@" );
			name.append( modname );
		}
		
	// モジュール => 先頭に '@' を付加する
	} else if ( typeid(node) == typeid(CNodeModule*) ) {
		m->modnamelist.push_back( name );
		
		name = string("@") + name;
		
	// システム変数 => 先頭に ` を付加する
	}
//	else if ( typeid(node) == typeid(CNodeSysvar*) ) { }
	
	insertItem(node, std::move(name), TVI_SORT);
	return;
}

//------------------------------------------------
// 戻り
//------------------------------------------------
void CTvCreator::procPost( IPolyNode* pNode )
{
	if ( typeid(pNode) == typeid(CNodeModule const*) ) {
		m->modnamelist.pop_back();
	}
	
	// 親ノードを戻す
	m->popParent();
	return;
}

//++++++++++++++++++++++++++++++++++++++++++++++++
//    リーフ
//++++++++++++++++++++++++++++++++++++++++++++++++
//------------------------------------------------
// 行き
//------------------------------------------------
void CTvCreator::procPre( ILeaf* pNode )
{
	/*
	TVINSERTSTRUCT& tvis = m->getTvIs();
	tvis.item.mask   |= TVIF_PARAM;
	tvis.item.lParam  = forcible_cast<LPARAM>( address_cast(pNode) );
	tvis.item.pszText = const_cast<char*>( pNode->getName().c_str() );
	
	// アイテムを挿入 (親にならないので、戻り値は保存しなくてもよい)
	TreeView_InsertItem( m->hTreeView, &tvis )
	//*/
	return;
}

}
