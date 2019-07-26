#include "pch.h"
#include "hsx_internals.h"

namespace hsp_sdk_ext {
	auto pval_to_type(PVal const* pval) -> HspType {
		assert(pval != nullptr);
		return (HspType)pval->flag;
	}

	auto pval_to_varmode(PVal const* pval) -> HspVarMode {
		assert(pval != nullptr);
		return (HspVarMode)pval->mode;
	}

	auto pval_to_lengths(PVal const* pval) -> HspDimIndex {
		assert(pval != nullptr);

		auto lengths = std::array<std::size_t, HspDimIndex::MAX_DIM>{};
		auto i = std::size_t{};

		while (i < HspDimIndex::MAX_DIM) {
			lengths[i] = pval->len[i + 1];

			if (i >= 1 && lengths[i] == 0) {
				break;
			}

			i++;
		}

		return HspDimIndex{ i, lengths };
	}

	auto pval_to_element_count(PVal const* pval) -> std::size_t {
		return pval_to_lengths(pval).size();
	}

	auto pval_is_standard_array(PVal const* pval, HSPCTX const* ctx) -> bool {
		assert(pval != nullptr);
		if (pval->len[1] == 1 && pval->len[2] == 0) {
			return false;
		}

		auto varproc = pval_to_varproc(pval, ctx);
		return varproc_does_support(varproc, HSPVAR_SUPPORT_FIXEDARRAY | HSPVAR_SUPPORT_FLEXARRAY);
	}

	// FIXME: HspData const を返す
	auto pval_to_data(PVal const* pval, HSPCTX const* ctx) -> HspData {
		auto vartype = pval_to_type(pval);
		auto aptr = 0;

		// FIXME: 関数に切り出す
		auto pval_mut = const_cast<PVal*>(pval);
		auto offset = pval->offset;
		pval_mut->offset = (APTR)aptr;
		auto pdat = pval_to_varproc(pval, ctx)->GetPtr(pval_mut);
		pval_mut->offset = offset;

		return HspData{ vartype, pdat };
	}
}
