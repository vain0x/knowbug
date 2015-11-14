
#pragma once

#include "Node.h"
#include "ITreeVisitor.h"

namespace DataTree
{;

// updated state をすべて None に貼り直す iterator
class CTreeUpdatedStateInitializer
	: public ITreeVisitor
{
public:
	void set(ITree* t) { t->setUpdatedState(ITree::UpdatedState::None); }

	void visit1(IMonoNode* t) { set(t); t->getChild(); }
	void visit1(IPolyNode* t) { set(t); for ( auto const child : t->getChildren() ) { visit0(child); } }
	void visit1(ILeaf* t) { set(t); }

	void visit0(ITree* t) override { t->acceptVisitor(*this); }

	void visit(CLoopNode* t)        override { visit1(t); }
	void visit(CNodeModule* t)      override { visit1(t); }
	void visit(CNodeVarArray* t)    override { visit1(t); }
	void visit(CNodeVarElem* t)     override { visit1(t); }
	void visit(CNodeVarHolder* t)   override { visit1(t); }
	void visit(CNodeModInst* t)     override { visit1(t); }
	void visit(CNodeModInstNull* t) override { visit1(t); }
	void visit(CNodePrmStk* t)      override { visit1(t); }
	void visit(CNodeLabel* t)       override { visit1(t); }
	void visit(CNodeString* t)      override { visit1(t); }
	void visit(CNodeDouble* t)      override { visit1(t); }
	void visit(CNodeInt* t)         override { visit1(t); }
	void visit(CNodeUnknown* t)     override { visit1(t); }
};

// あるノードの直接の祖先と子孫ノードを更新する
// 祖先は Shallow に(そのノードだけ)、子孫は Deep に(すべての子孫も含む)更新
class CTreeUpdater
	: public ITreeVisitor
{
public:
	CTreeUpdater(ITree* base)
		: base_(base), bNowDescendant_(true)
	{ }

	void visit0(ITree* t) override { t->acceptVisitor(*this); }

	void visit(CLoopNode*)        override;
	void visit(CNodeModule*)      override;
	void visit(CNodeVarArray*)    override;
	void visit(CNodeVarElem*)     override;
	void visit(CNodeVarHolder*)   override;
	void visit(CNodeModInst*)     override;
	void visit(CNodeModInstNull*) override;
	void visit(CNodePrmStk*)      override;
	void visit(CNodeLabel*)       override;
	void visit(CNodeString*)      override;
	void visit(CNodeDouble*)      override;
	void visit(CNodeInt*)         override;
	void visit(CNodeUnknown*)     override;

private:
	void visitParent(INodeContainer* t)  {
		bool bak = bNowDescendant_; bNowDescendant_ = bNowDescendant_ && (t != base_);
		visit0(t->getParent());
		bNowDescendant_ = bak;
	}

	void visitAsChild(ITree* child) {
		bool bak = bNowDescendant_; bNowDescendant_ |= (child == base_);
		visit0(child);
		bNowDescendant_ = bak;
	}

	// 今 visit している場所が起点ノードかその子孫なら真、祖先なら偽。
	// なお祖先でも子孫でもないノードに visit することはない
	bool isNowDescendant() const { return bNowDescendant_; }

private:
	// 更新の起点
	ITree* base_;

	bool bNowDescendant_;
};

}

