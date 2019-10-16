#include "pch.h"
#include "hsx_internals.h"

namespace hsx {
	auto param_stack_to_param_data_count(HspParamStack const& param_stack) -> std::size_t {
		return struct_to_param_count(param_stack.struct_dat());
	}

	auto param_stack_to_param_data(HspParamStack const& param_stack, std::size_t param_index, HSPCTX const* ctx) -> std::optional<HspParamData> {
		if (param_index >= param_stack_to_param_data_count(param_stack)) {
			return std::nullopt;
		}

		auto&& param_opt = struct_to_params(param_stack.struct_dat(), ctx).get(param_index);
		if (!param_opt) {
			return std::nullopt;
		}
		auto offset = (**param_opt).offset;

		auto ptr = (void*)((char const*)param_stack.ptr() + offset);
		return std::make_optional(HspParamData{ *param_opt, param_index, ptr, param_stack.safety() });
	}
}
