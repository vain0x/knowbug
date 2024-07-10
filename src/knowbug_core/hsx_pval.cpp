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

	auto pval_to_lengths(PVal const* pval) -> HsxIndexes {
		assert(pval != nullptr);

		auto lengths = HsxIndexes{ HSX_MAX_DIM, { 1, 0, 0, 0 } };
		auto d = std::size_t{};

		while (d < HSX_MAX_DIM) {
			lengths.data[d] = pval->len[d + 1];

			if (d >= 1 && lengths.data[d] == 0) {
				break;
			}

			d++;
		}

		lengths.dim = d;
		return lengths;
	}

	auto pval_to_element_count(PVal const* pval) -> std::size_t {
		return hsx_indexes_get_total(pval_to_lengths(pval));
	}

	auto pval_is_standard_array(PVal const* pval, HSPCTX const* ctx) -> bool {
		assert(pval != nullptr);
		if (pval->len[1] == 1 && pval->len[2] == 0) {
			return false;
		}

		auto varproc = pval_to_varproc(pval, ctx);
		return varproc_does_support(varproc, HSPVAR_SUPPORT_FIXEDARRAY | HSPVAR_SUPPORT_FLEXARRAY);
	}

	auto pval_to_data(PVal const* pval, HSPCTX const* ctx) -> std::optional<HsxData> {
		return element_to_data(pval, 0, ctx);
	}

	auto pval_to_memory_block(PVal const* pval, HSPCTX const* ctx) -> MemoryView {
		assert(pval != nullptr);

		auto data_opt = pval_to_data(pval, ctx);
		if (!data_opt) {
			return MemoryView{};
		}

		return element_data_to_memory_block(pval, data_opt->pdat, ctx);
	}

	auto pval_to_str(PVal const* pval, HSPCTX const* ctx)->std::optional<HsxStrSpan> {
		return element_to_str(pval, 0, ctx);
	}
}

HsxVarMetadata hsx_var_metadata_none() {
	return HsxVarMetadata{
		HSPVAR_FLAG_NONE,
		HSPVAR_MODE_NONE,
		HsxIndexes{ 1, { 1, 0, 0, 0 } },
		0,
		nullptr,
		nullptr,
		nullptr,
		0,
	};
}

HsxVarMetadata hsx_pval_to_var_metadata(PVal const* pval, HSPCTX const* ctx) {
	auto block_memory = hsx::pval_to_memory_block(pval, ctx);

	auto metadata = HsxVarMetadata{};
	metadata.vartype = hsx::pval_to_type(pval);
	metadata.varmode = hsx::pval_to_varmode(pval);
	metadata.lengths = hsx::pval_to_lengths(pval);
	metadata.data_size = pval->size;
	metadata.data_ptr = pval->pt;
	metadata.master_ptr = pval->master;
	metadata.block_ptr = block_memory.data();
	metadata.block_size = block_memory.size();
	return metadata;
}
