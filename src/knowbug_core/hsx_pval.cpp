#include "pch.h"
#include "hsx_internals.h"

namespace hsx {
	auto pval_to_type(PVal const* pval) -> HsxVartype {
		assert(pval != nullptr);
		return pval->flag;
	}

	auto pval_to_varmode(PVal const* pval) -> HsxVarMode {
		assert(pval != nullptr);
		return pval->mode;
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

	auto pval_to_data(PVal const* pval, HSPCTX const* ctx) -> std::optional<HspData> {
		return element_to_data(pval, 0, ctx);
	}

	auto pval_to_memory_block(PVal const* pval, HSPCTX const* ctx) -> MemoryView {
		assert(pval != nullptr);

		auto data_opt = pval_to_data(pval, ctx);
		if (!data_opt) {
			return MemoryView{};
		}

		return element_data_to_memory_block(pval, data_opt->ptr(), ctx);
	}

	auto pval_to_str(PVal const* pval, HSPCTX const* ctx)->std::optional<HsxStrSpan> {
		return element_to_str(pval, 0, ctx);
	}
}
