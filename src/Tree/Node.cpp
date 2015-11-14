
#include "../main.h"
#include "../DebugInfo.h"
#include "../module/ptr_cast.h"
#include "../module/strf.h"

#include "Node.h"

namespace DataTree {

static bool vartype_isKnown(vartype_t vt)
{
	return HSPVAR_FLAG_LABEL <= vt && vt <= HSPVAR_FLAG_INT;
}

// ŠÏ‘ªŽÒ
std::vector<TreeObservers> stt_observers;
void registerObserver(TreeObservers r) { stt_observers.push_back(r); }
static std::vector<TreeObservers>& getObservers() { return stt_observers; }

tree_t INode::addChild(tree_t child)
{
	for ( auto& r : getObservers() ) {
		r.appendObserver->visit0(child);
	}
	children_.push_back(child);
	return child;
}

tree_t INode::replaceChild(children_t::iterator& pos, tree_t child)
{
	removeChild(pos);
	*pos = child;
	return child;
}

void INode::removeChild(children_t::iterator& pos)
{
	auto& child = *pos;
	for ( auto& r : getObservers() ) {
		r.removeObserver->visit0(child);
	}
	delete child; child = nullptr;
}

void INode::removeChildAll()
{
	for ( auto iter = children_.begin(); iter != children_.end(); ++iter) {
		removeChild(iter);
	}
}

}
