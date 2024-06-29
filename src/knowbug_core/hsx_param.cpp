#include "pch.h"
#include "hsx_internals.h"

namespace hsx {
	auto params(HSPCTX const* ctx) -> Slice<STRUCTPRM> {
		auto size = (std::size_t)std::max(0, ctx->hsphed->max_minfo) / sizeof(STRUCTPRM);
		return Slice<STRUCTPRM>{ ctx->mem_minfo, size };
	}

	auto param_to_type(STRUCTPRM const* param)->HsxMptype {
		return param->mptype;
	}

	auto param_to_struct(STRUCTPRM const* param, HSPCTX const* ctx)->std::optional<STRUCTDAT const*> {
		auto struct_index = (std::size_t)std::max(0, (int)param->subid);
		return structs(ctx).get(struct_index);
	}

	auto param_type_to_name(HsxMptype param_type) -> std::optional<char const*> {
		switch (param_type) {
		case MPTYPE_LABEL:
			return "label";

		case MPTYPE_DNUM:
			return "double";

		case MPTYPE_LOCALSTRING:
			return "str";

		case MPTYPE_INUM:
			return "int";

		case MPTYPE_SINGLEVAR:
			return "var";

		case MPTYPE_ARRAYVAR:
			return "array";

		case MPTYPE_LOCALVAR:
			return "local";

		case MPTYPE_MODULEVAR:
		case MPTYPE_IMODULEVAR:
		case MPTYPE_TMODULEVAR:
			return "modvar";

		default:
			return std::nullopt;
		}
	}
}
