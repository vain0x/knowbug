#pragma once

#include "Node_fwd.h"
#include "Visitor.h"

namespace DataTree
{;

template<class TVisitor>
class IObserver
	: public IVisitor
{
public:
	IObserver(TVisitor& cb)
		: cb_(cb)
	{}

	TVisitor& getCallback() const { return cb_; }

	void visit(NodeRoot*)   override {}
	void visit(NodeLoop*)   override {}
	void visit(NodeModule*) override {}
	void visit(NodeArray*)  override {}
	void visit(NodeValue*)  override {}

private:
	TVisitor& cb_;
};

}
