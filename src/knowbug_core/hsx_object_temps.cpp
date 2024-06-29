#include "pch.h"
#include "hsx_data.h"
#include "hsx_internals.h"

namespace hsx {
	auto object_temps(HSPCTX const* ctx) -> Slice<HsxObjectTemp> {
		auto size = (std::size_t)std::max(0, ctx->hsphed->max_ot) / sizeof(HsxObjectTemp);
		return Slice<HsxObjectTemp>{ ctx->mem_ot, size };
	}

	auto object_temp_count(HSPCTX const* ctx) -> std::size_t {
		return object_temps(ctx).size();
	}

	auto object_temp_to_label(std::size_t ot_index, HSPCTX const* ctx) -> std::optional<HsxLabel> {
		auto offset_opt = object_temps(ctx).get(ot_index);
		if (!offset_opt) {
			return std::nullopt;
		}

		// OT 領域の値は CS 領域のオフセットを表しているので、
		// この値を CS 領域の先頭ポインタに加算すると、
		// CS 上でラベルが指す領域へのポインタが得られる。
		auto label = ctx->mem_mcs + **offset_opt;

		return std::make_optional(label);
	}
}
