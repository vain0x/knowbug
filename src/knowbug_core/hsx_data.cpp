#include "pch.h"
#include "hsx_internals.h"

namespace hsx {
	auto data_from_label(HsxLabel const* ptr) -> HsxData {
		return HsxData{ HSPVAR_FLAG_LABEL, (PDAT const*)ptr };
	}

	auto data_from_str(HsxStrPtr ptr) -> HsxData {
		return HsxData{ HSPVAR_FLAG_STR, (PDAT const*)ptr };
	}

	auto data_from_double(HsxDouble const* ptr) -> HsxData {
		return HsxData{ HSPVAR_FLAG_DOUBLE, (PDAT const*)ptr };
	}

	auto data_from_int(HsxInt const* ptr) -> HsxData {
		return HsxData{ HSPVAR_FLAG_INT, (PDAT const*)ptr };
	}

	auto data_from_flex(FlexValue const* flex) -> HsxData {
		return HsxData{ HSPVAR_FLAG_STRUCT, (PDAT const*)flex };
	}

	auto data_to_label(HsxData const& data)->std::optional<HsxLabel> {
		if (data.vartype != HSPVAR_FLAG_LABEL) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE(*(HsxLabel const*)data.pdat));
	}

	auto data_to_str(HsxData const& data)->std::optional<char const*> {
		if (data.vartype != HSPVAR_FLAG_STR) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE((char const*)data.pdat));
	}

	auto data_to_double(HsxData const& data)->std::optional<HsxDouble> {
		if (data.vartype != HSPVAR_FLAG_DOUBLE) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE(*(HsxDouble const*)data.pdat));
	}

	auto data_to_int(HsxData const& data)->std::optional<HsxInt> {
		if (data.vartype != HSPVAR_FLAG_INT) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE(*(HsxInt const*)data.pdat));
	}

	auto data_to_flex(HsxData const& data)->std::optional<FlexValue const*> {
		if (data.vartype != HSPVAR_FLAG_STRUCT) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE((FlexValue const*)data.pdat));
	}
}
