// visitor - Treeview-Node 生成者

#ifndef IG_CLASS_TREEVIEW_NODE_CREATOR_H
#define IG_CLASS_TREEVIEW_NODE_CREATOR_H

#include <memory>
#include <cstdarg>
#include <windows.h>
#include <CommCtrl.h>

#include "ITreeVisitor.h"

namespace DataTree
{

//##############################################################################
//                宣言部 : CTvCreator
//##############################################################################
//------------------------------------------------
// ツリービュー・ノードの生成者
//------------------------------------------------
class CTvCreator
	: public ITreeVisitor
{
public:
	CTvCreator( HWND hTreeView );
	
	//******************************************************
	//    インターフェース
	//******************************************************
public:
	void visit(tree_t);
	
	//******************************************************
	//    ノード処理の実装
	//******************************************************
private:
	// 一般
//	virtual void procPre (ITree*) { }
//	virtual void procEach(ITree*) { }
//	virtual void procPost(ITree*) { }
	
private:
	// コンテナ
	virtual void     procPre (IPolyNode*);
	virtual bool requiresEach(IPolyNode*) const { return false; }
	virtual void     procPost(IPolyNode*);
	
	// リーフ
	virtual void     procPre (ILeaf*);
	virtual bool requiresEach(ILeaf*) const { return false; }
	virtual bool requiresPost(ILeaf*) const { return false; }
	
	//******************************************************
	//    内部メンバ関数
	//******************************************************
private:
	void CTvCreator::insertItem(tree_t, string name, HTREEITEM hInsertAfter);

	//******************************************************
	//    メンバ変数
	//******************************************************
private:
	struct Impl;
	std::unique_ptr<Impl> m;
};

}

#endif
