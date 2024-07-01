#include "pch.h"
#include "hsx_internals.h"

namespace hsx {
	auto element_to_indexes(PVal const* pval, std::size_t aptr) -> std::optional<HsxIndexes> {
		auto count = pval_to_element_count(pval);
		if (aptr >= count) {
			return std::nullopt;
		}

		auto lengths = pval_to_lengths(pval);

		// E.g. lengths=(2, 3), aptr=5, indexes=(1, 2)
		auto indexes = HsxIndexes{ lengths.dim, { 1, 0, 0, 0 } };

		for (auto i = std::size_t{}; i < lengths.dim; i++) {
			indexes.data[i] = aptr % lengths.data[i];
			aptr /= lengths.data[i];
		}

		return std::make_optional(indexes);
	}

	auto element_to_aptr(PVal const* pval, HsxIndexes indexes) -> std::optional<std::size_t> {
		auto lengths = pval_to_lengths(pval);

		auto unit = std::size_t{ 1 };
		auto aptr = std::size_t{};
		for (auto i = std::size_t{}; i < lengths.dim; i++) {
			if (indexes.data[i] >= lengths.data[i]) {
				return std::nullopt;
			}

			aptr += indexes.data[i] * unit;
			unit *= lengths.data[i];
		}

		return std::make_optional(aptr);
	}

	auto element_to_data(PVal const* pval, std::size_t aptr, HSPCTX const* ctx) -> std::optional<HsxData> {
		auto count = pval_to_element_count(pval);
		if (aptr >= count) {
			return std::nullopt;
		}

		auto vartype = pval_to_type(pval);

		// 一時的に PVal::offset を書き換えて戻す。
		auto pval_mut = const_cast<PVal*>(pval);
		auto offset = pval->offset;
		pval_mut->offset = (APTR)aptr;
		auto pdat = pval_to_varproc(pval, ctx)->GetPtr(pval_mut);
		pval_mut->offset = offset;

		return std::make_optional(HsxData{ vartype, pdat });
	}

	auto element_to_memory_block(PVal const* pval, std::size_t aptr, HSPCTX const* ctx) -> MemoryView {
		assert(pval != nullptr);

		auto data_opt = element_to_data(pval, aptr, ctx);
		if (!data_opt) {
			return MemoryView{};
		}

		return element_data_to_memory_block(pval, data_opt->pdat, ctx);
	}

	auto element_to_str(PVal const* pval, std::size_t aptr, HSPCTX const* ctx) -> std::optional<HsxStrSpan> {
		assert(pval != nullptr);

		auto memory = element_to_memory_block(pval, aptr, ctx);
		return HsxStrSpan{ (char const*)memory.data(), memory.size() };
	}
}
