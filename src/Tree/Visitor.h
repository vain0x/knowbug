#pragma once
#include "Node_fwd.h"

namespace DataTree
{

class IVisitor
{
public:
	virtual ~IVisitor() { }

	virtual void visit0(tree_t) = 0;
	virtual void visit(NodeRoot*) = 0;
	virtual void visit(NodeLoop*) = 0;
	virtual void visit(NodeModule*) = 0;
	virtual void visit(NodeArray*) = 0;
	virtual void visit(NodeValue*) = 0;
	/*
	virtual void visit(NodeModInst*) = 0;
	virtual void visit(NodePrmStk*) = 0;
	//*/
};

}
