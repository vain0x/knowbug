#pragma once

#include <Windows.h>
#include <CommCtrl.h>
#include <memory>

#include "Visitor.h"
#include "Observer.h"
#include "Node.h"

namespace DataTree
{

/**
ノードの変更を監視してツリービューに反映させるもの
//*/
class TvRepr
{
public:
	TvRepr(HWND hTv);
	~TvRepr();

	HTREEITEM tryFindTvItem(tree_t node) const;
	tree_t tryFindNode(HTREEITEM hItem) const;

	template<class TNode>
	TNode* tryFindNode(HTREEITEM node) const
	{
		return dynamic_cast<TNode*>(tryFindNode(node));
	}

private:
	void spawnRoot(NodeRoot* root);
	void TvRepr::insertItem(tree_t node, string name, HTREEITEM hParent, HTREEITEM hInsertAfter);

	friend class TvAppendObserver;
	friend class TvRemoveObserver;

	class TvAppendObserver
		: public IObserver<TvRepr>
	{
		friend class TvRepr;
		TvAppendObserver(TvRepr& r) : IObserver(r) {}

	public:
		void visit0(tree_t t) override;
	};

	class TvRemoveObserver
		: public IObserver<TvRepr>
	{
		friend class TvRepr;
		TvRemoveObserver(TvRepr& r) : IObserver(r) { }

	public:
		void visit0(tree_t t) override;
	};

	struct Impl;
	std::unique_ptr<Impl> m;
};

}
