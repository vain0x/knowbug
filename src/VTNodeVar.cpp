
#include "main.h"
#include "VarTreeNodeData.h"
#include "module/strf.h"

void VTNodeVector::addChild(int i)
{
	if ( dimIndex_ == 0 ) {
		// TODO: 分かりやすい名前をつける
		auto&& name = strf("(%d)", i);

		children_.emplace_back(std::make_shared<VTNodeValue>
			( this, std::move(name)
			, hpiutil::PVal_getPtr(pval_, aptr_ + i), pval_->flag
			));
	} else {
		// TODO: 多次元配列を実装
	}
}

bool VTNodeVector::updateSub(bool deep)
{
	if ( deep ) {
		int const newLen = pval_->len[1 + dimIndex_];

		// var の子ノード管理の仕組みにより、これの要素数が減ったり、型が変わることはありえない
		// (そうなったらこれ自体が破棄される)
		assert(len_ <= newLen);

		for ( int i = len_; i < newLen; i++ ) {
			addChild(i);
		}

		for ( auto& e : children_ ) {
			e->updateDownDeep();
		}
	}
	return true;
}
