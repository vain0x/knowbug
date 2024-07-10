#include "pch.h"
#include "hsx_internals.h"

namespace hsx {
	auto param_stack_to_param_data_count(HsxParamStack const& param_stack) -> std::size_t {
		return struct_to_param_count(param_stack.struct_dat);
	}

	auto param_stack_to_param_data(HsxParamStack const& param_stack, std::size_t param_index, HSPCTX const* ctx) -> std::optional<HsxParamData> {
		if (param_index >= param_stack_to_param_data_count(param_stack)) {
			return std::nullopt;
		}

		auto param_opt = struct_to_params(param_stack.struct_dat, ctx).get(param_index);
		if (!param_opt) {
			return std::nullopt;
		}
		auto offset = (**param_opt).offset;

		auto ptr = (void*)((char const*)param_stack.stack_ptr + offset);

		assert(*param_opt != nullptr);
		assert(ptr != nullptr);
		return std::make_optional(HsxParamData{ *param_opt, param_index, ptr, param_stack.safety });
	}
}
