#include "pch.h"
#include "hsx_data.h"
#include "hsx_internals.h"

namespace hsp_sdk_ext {
	auto params(HSPCTX const* ctx) -> Slice<STRUCTPRM> {
		auto size = (std::size_t)std::max(0, ctx->hsphed->max_minfo) / sizeof(STRUCTPRM);
		return Slice<STRUCTPRM>{ ctx->mem_minfo, size };
	}
}
