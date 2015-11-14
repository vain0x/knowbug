// Visitor

#ifndef IG_INTERFACE_DATATREE_VISITOR_H
#define IG_INTERFACE_DATATREE_VISITOR_H

#include "Node.dec.h"

namespace DataTree
{;

	/*
	// 以前の実装では iterator だった
#define ITreeVisitorDecProcMembersLeaf(T) \
	virtual void procPre(T);
#define ITreeVisitorDecProcMembersMono(T) \
	virtual void procPre(T); virtual void procPost(T);
#define ITreeVisitorDecProcMembersPoly(T) \
	virtual void procPre(T); virtual void procEach(T); virtual void procPost(T);
	
	// 一般
//	ITreeVisitorDecProcMembers(target_t);
	virtual void procPre (target_t) { };	// 行き
	virtual void procEach(target_t) { };	// 各子ノードに visit する直前 (子ノードのポインタも渡した方がいいのでは？)
	virtual void procPost(target_t) { };	// 戻り
	//*/

class ITreeVisitor
{
public:
	virtual ~ITreeVisitor() { }

	virtual void visit0(ITree*) = 0;	// t->acceptVisitor(*this);
	virtual void visit(CLoopNode*) = 0;

	virtual void visit(CNodeModule*) = 0;
	virtual void visit(CNodeVarHolder*) = 0;
	virtual void visit(CNodeVarArray*) = 0;
	virtual void visit(CNodeVarElem*) = 0;

	virtual void visit(CNodeModInst*) = 0;
	virtual void visit(CNodeModInstNull*) = 0;
	virtual void visit(CNodePrmStk*) = 0;

	virtual void visit(CNodeLabel*) = 0;
	virtual void visit(CNodeString*) = 0;
	virtual void visit(CNodeDouble*) = 0;
	virtual void visit(CNodeInt*) = 0;
	virtual void visit(CNodeUnknown*) = 0;
};

}
#endif
