
#include "main.h"
#include "VarTreeNodeData.h"
#include "module/strf.h"

bool VTNodeVar::updateSub(bool deep)
{
	if ( deep ) {
		// 型が変わるか、要素数が減るか、低次元の要素数が増えたなら、再構築を行う
		if ( pval_->flag != pvalBak_.flag
			|| memcmp(&pvalBak_.len[1], &pval_->len[1], hpiutil::ArrayDimMax * sizeof(int))
				> 0
			) {
			children_.clear();
		}

		// 既存要素は更新し、新規要素は挿入する
		{
			size_t const oldLen = children_.size();
			size_t const newLen = hpiutil::PVal_cntElems(pval_);

			for ( size_t i = 0; i < newLen; i++ ) {
				PDAT const* const pdat = hpiutil::PVal_getPtr(pval_, i);

				if ( i < oldLen ) {
					children_[i]->resetPtr(pdat);

				} else {
					auto&& name =
						hpiutil::stringifyArrayIndex(hpiutil::PVal_indexesFromAptr(pval_, i));

					children_.emplace_back(std::make_shared<VTNodeValue>
						( this, std::move(name)
						, pdat, pval_->flag
						));
				}

				children_[i]->updateDownDeep();
			}
		}
	}
	pvalBak_ = *pval_;
	return true;
}

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
