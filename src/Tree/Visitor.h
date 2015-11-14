#pragma once
#include "Node.dec.h"

namespace DataTree
{

class IVisitor
{
public:
	virtual ~IVisitor() { }

	virtual void visit0(tree_t) = 0;
	virtual void visit(NodeLoop*) = 0;
	virtual void visit(NodeModule*) = 0;
	virtual void visit(NodeArray*) = 0;

	virtual void visit(NodeLabel*) = 0;
	virtual void visit(NodeString*) = 0;
	virtual void visit(NodeDouble*) = 0;
	virtual void visit(NodeInt*) = 0;
	virtual void visit(NodeUnknown*) = 0;
	/*
	virtual void visit(NodeModInst*) = 0;
	virtual void visit(NodeModInstNull*) = 0;
	virtual void visit(NodePrmStk*) = 0;
	//*/
};

}
