#include "pch.h"
#include "hsx_internals.h"

namespace hsx {
	auto data_from_label(HsxLabel const* ptr) -> HspData {
		return HspData{ HspType::Label, (PDAT const*)ptr };
	}

	auto data_from_str(HsxStrPtr ptr) -> HspData {
		return HspData{ HspType::Str, (PDAT const*)ptr };
	}

	auto data_from_double(HspDouble const* ptr) -> HspData {
		return HspData{ HspType::Double, (PDAT const*)ptr };
	}

	auto data_from_int(HspInt const* ptr) -> HspData {
		return HspData{ HspType::Int, (PDAT const*)ptr };
	}

	auto data_from_flex(FlexValue const* flex) -> HspData {
		return HspData{ HspType::Struct, (PDAT const*)flex };
	}

	auto data_to_label(HspData const& data)->std::optional<HsxLabel> {
		if (data.type() != HspType::Label) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE(*(HsxLabel const*)data.ptr()));
	}

	auto data_to_str(HspData const& data)->std::optional<char const*> {
		if (data.type() != HspType::Str) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE((char const*)data.ptr()));
	}

	auto data_to_double(HspData const& data)->std::optional<HspDouble> {
		if (data.type() != HspType::Double) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE(*(HspDouble const*)data.ptr()));
	}

	auto data_to_int(HspData const& data)->std::optional<HspInt> {
		if (data.type() != HspType::Int) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE(*(HspInt const*)data.ptr()));
	}

	auto data_to_flex(HspData const& data)->std::optional<FlexValue const*> {
		if (data.type() != HspType::Struct) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE((FlexValue const*)data.ptr()));
	}
}
