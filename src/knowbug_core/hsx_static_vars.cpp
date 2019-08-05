#include "pch.h"
#include "hsx_internals.h"

#undef max

namespace hsp_sdk_ext {
	auto static_vars(HSPCTX const* ctx)->Slice<PVal> {
		auto size = (std::size_t)std::max(0, ctx->hsphed->max_val);
		return Slice<PVal>{ ctx->mem_var, size };
	}

	auto static_var_count(HSPCTX const* ctx)->std::size_t {
		return static_vars(ctx).size();
	}

	auto static_var_to_pval(std::size_t static_var_index, HSPCTX const* ctx)->std::optional<PVal const*> {
		return static_vars(ctx).get(static_var_index);
	}

	auto static_var_from_name(char const* var_name, HSPCTX const* ctx)->std::optional<std::size_t> {
		if (!var_name) {
			assert(false && u8"var_name must not be null");
			return std::nullopt;
		}

		auto index = exinfo(ctx)->HspFunc_seekvar(var_name);
		if (index < 0 || (std::size_t)index >= static_var_count(ctx)) {
			return std::nullopt;
		}

		return std::make_optional((std::size_t)index);
	}

	auto static_var_to_name(std::size_t static_var_index, HSPCTX const* ctx)->std::optional<char const*> {
		if (static_var_index >= static_var_count(ctx)) {
			assert(false && u8"invalid static_var_index");
			return std::nullopt;
		}

		auto var_name = exinfo(ctx)->HspFunc_varname((int)static_var_index);
		if (!var_name) {
			assert(false && u8"var_name should be found");
			return std::nullopt;
		}

		return std::make_optional(var_name);
	}
}
