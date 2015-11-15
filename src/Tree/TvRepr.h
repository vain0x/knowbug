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
	void spawnRoot(NodeGlobal* node);
	void TvRepr::insertItem(tree_t node, string name, HTREEITEM hParent, HTREEITEM hInsertAfter);

	friend class TvAppendObserver;
	friend class TvRemoveObserver;

	class TvAppendObserver
		: public IObserver<TvRepr>
	{
		friend class TvRepr;
		TvAppendObserver(TvRepr& r) : IObserver(r) {}

	public:
		void visit0(tree_t t) override { t->acceptVisitor(*this); }

		void visit(NodeLoop* t)     override { visit1(t); }
		void visit(NodeModule* t)   override { visit1(t); }
		void visit(NodeArray* t)    override { visit1(t); }
		void visit(NodeLabel* t)    override { visit1(t); }
		void visit(NodeString* t)   override { visit1(t); }
		void visit(NodeDouble* t)   override { visit1(t); }
		void visit(NodeInt* t)      override { visit1(t); }
		void visit(NodeUnknown* t)  override { visit1(t); }

		void visit1(INode* t);
		void visit1(ILeaf*) { }
	};

	class TvRemoveObserver
		: public IObserver<TvRepr>
	{
		friend class TvRepr;
		TvRemoveObserver(TvRepr& r) : IObserver(r) { }

	public:
		void visit0(tree_t t) override { t->acceptVisitor(*this); }

		void visit(NodeLoop* t)     override { visit1(t); }
		void visit(NodeModule* t)   override { visit1(t); }
		void visit(NodeArray* t)    override { visit1(t); }
		void visit(NodeLabel* t)    override { visit1(t); }
		void visit(NodeString* t)   override { visit1(t); }
		void visit(NodeDouble* t)   override { visit1(t); }
		void visit(NodeInt* t)      override { visit1(t); }
		void visit(NodeUnknown* t)  override { visit1(t); }

		void visit1(INode* t);
		void visit1(ILeaf*) {}
	};

	struct Impl;
	std::unique_ptr<Impl> m;
};

}
