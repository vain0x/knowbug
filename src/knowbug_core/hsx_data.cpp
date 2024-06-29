#include "pch.h"
#include "hsx_internals.h"

namespace hsx {
	auto data_from_label(HsxLabel const* ptr) -> HspData {
		return HspData{ HSPVAR_FLAG_LABEL, (PDAT const*)ptr };
	}

	auto data_from_str(HsxStrPtr ptr) -> HspData {
		return HspData{ HSPVAR_FLAG_STR, (PDAT const*)ptr };
	}

	auto data_from_double(HsxDouble const* ptr) -> HspData {
		return HspData{ HSPVAR_FLAG_DOUBLE, (PDAT const*)ptr };
	}

	auto data_from_int(HsxInt const* ptr) -> HspData {
		return HspData{ HSPVAR_FLAG_INT, (PDAT const*)ptr };
	}

	auto data_from_flex(FlexValue const* flex) -> HspData {
		return HspData{ HSPVAR_FLAG_STRUCT, (PDAT const*)flex };
	}

	auto data_to_label(HspData const& data)->std::optional<HsxLabel> {
		if (data.type() != HSPVAR_FLAG_LABEL) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE(*(HsxLabel const*)data.ptr()));
	}

	auto data_to_str(HspData const& data)->std::optional<char const*> {
		if (data.type() != HSPVAR_FLAG_STR) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE((char const*)data.ptr()));
	}

	auto data_to_double(HspData const& data)->std::optional<HsxDouble> {
		if (data.type() != HSPVAR_FLAG_DOUBLE) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE(*(HsxDouble const*)data.ptr()));
	}

	auto data_to_int(HspData const& data)->std::optional<HsxInt> {
		if (data.type() != HSPVAR_FLAG_INT) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE(*(HsxInt const*)data.ptr()));
	}

	auto data_to_flex(HspData const& data)->std::optional<FlexValue const*> {
		if (data.type() != HSPVAR_FLAG_STRUCT) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE((FlexValue const*)data.ptr()));
	}
}
