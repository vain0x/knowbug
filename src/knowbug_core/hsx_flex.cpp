#include "pch.h"
#include "hsx_internals.h"

namespace hsp_sdk_ext {
	static auto flex_to_struct_tag(FlexValue const* flex, HSPCTX const* ctx) -> std::optional<STRUCTPRM const*> {
		if (flex_is_nullmod(flex)) {
			return std::nullopt;
		}
		return params(ctx).get(flex->customid);
	}

	auto flex_is_nullmod(FlexValue const* flex) -> bool {
		assert(flex != nullptr);
		return !flex->ptr || flex->type == FLEXVAL_TYPE_NONE;
	}

	auto flex_is_clone(FlexValue const* flex) -> bool {
		assert(flex != nullptr);
		return flex->type == FLEXVAL_TYPE_CLONE;
	}

	auto flex_to_struct(FlexValue const* flex, HSPCTX const* ctx)->std::optional<STRUCTDAT const*> {
		auto&& struct_tag_opt = flex_to_struct_tag(flex, ctx);
		if (!struct_tag_opt) {
			return std::nullopt;
		}

		return param_to_struct(*struct_tag_opt, ctx);
	}

	auto flex_to_member_count(FlexValue const* flex, HSPCTX const* ctx) -> std::size_t {
		assert(flex != nullptr);

		auto&& struct_opt = flex_to_struct(flex, ctx);
		if (!struct_opt) {
			return 0;
		}

		auto param_count = struct_to_param_count(*struct_opt);

		// NOTE: STRUCT_TAG というダミーのパラメータがあるため、メンバ変数の個数はパラメータ数より 1 少ない。
		if (param_count == 0) {
			assert(false && u8"STRUCT_TAG パラメータを持たない struct はインスタンスを作れないはず");
			return 0;
		}
		return param_count - 1;
	}

	auto flex_to_member(FlexValue const* flex, std::size_t member_index, HSPCTX const* ctx) -> std::optional<HspParamData> {
		auto member_count = flex_to_member_count(flex, ctx);
		if (member_index >= member_count) {
			assert(false && u8"Invalid member_index");
			return std::nullopt;
		}

		// 先頭の STRUCT_TAG を除いて数える。
		auto param_index = member_index + 1;

		auto&& param_stack_opt = flex_to_param_stack(flex, ctx);
		if (!param_stack_opt) {
			return std::nullopt;
		}

		auto&& param_data_opt = param_stack_to_param_data(*param_stack_opt, param_index, ctx);
		if (!param_data_opt) {
			assert(false && u8"Invalid member_index");
			return std::nullopt;
		}

		return *param_data_opt;
	}

	auto flex_to_param_stack(FlexValue const* flex, HSPCTX const* ctx) -> std::optional<HspParamStack> {
		auto&& struct_opt = flex_to_struct(flex, ctx);
		if (!struct_opt) {
			return std::nullopt;
		}

		auto size = struct_to_param_stack_size(*struct_opt);
		auto safe = true;
		return std::make_optional<HspParamStack>(*struct_opt, flex->ptr, size, safe);
	}
}
