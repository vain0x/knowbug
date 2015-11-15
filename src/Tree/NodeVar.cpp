#pragma once

#include "../module/ptr_cast.h"
#include "hpimod/stringization.h"
#include "Node.h"

namespace DataTree {

NodeArray::NodeArray(tree_t parent, string const& name, PVal* pval)
	: Node(parent, name)
	, pval_(pval)
	, cur_ {}
{}

void NodeArray::addElem(size_t aptr)
{
	auto&& name = hpimod::stringizeArrayIndex(hpimod::PVal_indexesFromAptr(pval_, aptr));
	auto&& pdat = hpimod::PVal_getPtr(pval_, aptr);
	addChild(new NodeValue(this, std::move(name), pdat, pval_->flag));
}

void NodeArray::updateElem(size_t aptr)
{
	assert(aptr < getChildren().size());
	auto&& child = getChildren()[aptr];
	assert(dynamic_cast<NodeValue*>(child) != nullptr);
	static_cast<NodeValue*>(child)->setValptr(hpimod::PVal_getPtr(pval_, aptr));
}

bool NodeArray::updateState(tree_t childOrNull)
{
	if ( !getParent()->updateState(this) ) return false;

	// 前回の更新時とはデータに連続性がないと思われる場合
	bool const rebuilt =
		( cur_.mode != pval_->mode
		|| cur_.flag != pval_->flag
		|| (hpimod::PVal_cntElems(pval_) < hpimod::PVal_cntElems(&cur_))
		);

	if ( rebuilt ) {
		cur_ = PVal {};
		removeChildAll();
	}

	size_t const oldCntElems = hpimod::PVal_cntElems(&cur_);
	size_t const newCntElems = hpimod::PVal_cntElems(pval_);

	for ( size_t i = 0; i < newCntElems; ++i ) {
		if ( i < oldCntElems && !rebuilt && !childOrNull ) {
			updateElem(i);
		} else {
			addElem(i);
		}
	}

	cur_ = *pval_;
	return (!rebuilt);
}

}
