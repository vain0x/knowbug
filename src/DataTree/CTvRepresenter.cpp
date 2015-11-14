// ツリービュー表現者

#include "Node.h"
#include "CTvRepresenter.h"

#include "module/ptr_cast.h"
#include "module/strf.h"

#include <stack>
#include <vector>
#include <map>
#include <cstring>
#include <CommCtrl.h>
#include <windows.h>

#include "dialog.h"		// for TreeView_*

static TVINSERTSTRUCT& getTvIs(HTREEITEM hParent);

namespace DataTree
{
	//**********************************************************
	// メンバ変数
	//**********************************************************
	struct CTvRepresenter::Impl
	{
		HWND hTreeView;

		typedef std::map<tree_t, HTREEITEM> mapNodeToTvItem_t;
		mapNodeToTvItem_t mapNodeToTvItem;

		CTvAppendObserver appendObserver;
		CTvRemoveObserver removeObserver;
	public:
		Impl(HWND hTv, CTvAppendObserver _append, CTvRemoveObserver _remove)
			: hTreeView(hTv), appendObserver(_append), removeObserver(_remove)
		{ }
	};

	//**********************************************************
	// インターフェース
	//**********************************************************
	CTvRepresenter::CTvRepresenter(HWND hTreeView)
		: m(new Impl(hTreeView, CTvAppendObserver(this), CTvRemoveObserver(this)))
	{
		registerObserver({&m->appendObserver, &m->removeObserver});
		return;
	}

	CTvRepresenter::~CTvRepresenter()
	{
		delete m;
	}
	
	//**********************************************************
	// append
	//**********************************************************
	void CTvRepresenter::CTvAppendObserver::visit1(INodeContainer* newChild)
	{
		auto const parent = newChild->getParent();
		getOwner()->insertItem(newChild, newChild->getName(), getOwner()->findTvItem(parent), TVI_SORT);
		return;
	}

	//**********************************************************
	// remove
	//**********************************************************
	void CTvRepresenter::CTvRemoveObserver::visit1(INodeContainer* removed)
	{
		if ( auto const hItem = getOwner()->findTvItem(removed) ) {
			TreeView_DeleteItem(getOwner()->m->hTreeView, hItem);
		}
		return;
	}

	//**********************************************************
	// 内部メソッド
	//**********************************************************
	HTREEITEM CTvRepresenter::findTvItem(tree_t node) const {
		auto const iter = m->mapNodeToTvItem.find(node);
		return (iter != m->mapNodeToTvItem.end() ? iter->second : nullptr);
	}

	tree_t CTvRepresenter::findNode(HTREEITEM hItem) const {
		return reinterpret_cast<tree_t>(TreeView_GetItemLParam(m->hTreeView, hItem));
	}

	//------------------------------------------------
	// アイテムの挿入
	//------------------------------------------------
	void CTvRepresenter::insertItem(tree_t node, string name, HTREEITEM hParent, HTREEITEM hInsertAfter)
	{
		static TVINSERTSTRUCT tvis;
		//TVINSERTSTRUCT& tvis = getTvIs(hParent);
		std::memset(&tvis, 0x00, sizeof(tvis));
		tvis.hParent = hParent;
		tvis.hInsertAfter = hInsertAfter;
		tvis.item.mask = TVIF_TEXT;

		tvis.item.mask |= TVIF_PARAM;
		tvis.item.lParam = ctype_cast<LPARAM>(address_cast(node));
		tvis.item.pszText = const_cast<char*>(name.c_str());

		TreeView_InsertItem(m->hTreeView, &tvis);
		return;
	}


	/*
	//**********************************************************
	//    メンバ変数の実体
	//**********************************************************
	struct CTvRepresenter::Impl
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
		Impl(HWND _hTreeView)
			: hTreeView(_hTreeView)
		{
			modnamelist.reserve(1);
			pushParent(TVI_ROOT);		// 番兵
			pushParent(TVI_ROOT);
			return;
		}

		//--------------------------------------------
		// stkParent の操作
		//--------------------------------------------
		void pushParent(HTREEITEM hParent_)
		{
			stkParent.push(hParent_);
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
			std::memset(&tvis, 0x00, sizeof(tvis));
			tvis.hParent = hParent;
			tvis.hInsertAfter = TVI_SORT;	// TODO: 呼び出しノードなどはソートしてはいけない
			tvis.item.mask = TVIF_TEXT;
			return;
		}
	};

	//**********************************************************
	//    構築と解体
	//**********************************************************
	//------------------------------------------------
	// 構築
	//------------------------------------------------
	CTvRepresenter::CTvRepresenter(HWND hTreeView)
		: m(new Impl(hTreeView))
	{ }

	//------------------------------------------------
	// アイテムの挿入
	//------------------------------------------------
	void CTvRepresenter::insertItem(tree_t node, string name, HTREEITEM hInsertAfter)
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
	void CTvRepresenter::visit(tree_t node)
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
	void CTvRepresenter::procPre(IPolyNode* node)
	{
		string name(node->getName());

		// 変数 => スコープ解決を付加する
		if ( typeid(node) == typeid(CNodeVarArray*)
			|| typeid(node) == typeid(CNodeVarElem*)
			) {
			for each (string modname in m->modnamelist) {
				name.append("@");
				name.append(modname);
			}

			// モジュール => 先頭に '@' を付加する
		} else if ( typeid(node) == typeid(CNodeModule*) ) {
			m->modnamelist.push_back(name);

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
	void CTvRepresenter::procPost(IPolyNode* pNode)
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
	void CTvRepresenter::procPre(ILeaf* pNode)
	{
		/*
		TVINSERTSTRUCT& tvis = m->getTvIs();
		tvis.item.mask   |= TVIF_PARAM;
		tvis.item.lParam  = forcible_cast<LPARAM>( address_cast(pNode) );
		tvis.item.pszText = const_cast<char*>( pNode->getName().c_str() );

		// アイテムを挿入 (親にならないので、戻り値は保存しなくてもよい)
		TreeView_InsertItem( m->hTreeView, &tvis )
		//* /
		return;
	}
	//*/
}
