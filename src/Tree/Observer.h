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

private:
	TVisitor& cb_;
};

}
