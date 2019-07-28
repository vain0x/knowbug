#include "pch.h"
#include "hsx_internals.h"

namespace hsp_sdk_ext {
	auto data_from_label(HspLabel const* ptr) -> HspData {
		return HspData{ HspType::Label, (PDAT*)ptr };
	}

	auto data_from_str(char const* value) -> HspData {
		return HspData{ HspType::Str, (PDAT*)value };
	}

	auto data_from_double(HspDouble const* ptr) -> HspData {
		return HspData{ HspType::Double, (PDAT*)ptr };
	}

	auto data_from_int(HspInt const* ptr) -> HspData {
		return HspData{ HspType::Int, (PDAT*)ptr };
	}

	auto data_from_flex(FlexValue const* flex) -> HspData {
		return HspData{ HspType::Struct, (PDAT*)flex };
	}

	auto data_to_label(HspData const& data)->std::optional<HspLabel> {
		if (data.type() != HspType::Label) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE(*(HspLabel const*)data.ptr()));
	}

	auto data_to_str(HspData const& data)->std::optional<HspStr> {
		if (data.type() != HspType::Str) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE((HspStr)data.ptr()));
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

	auto data_to_flex(HspData const& data)->std::optional<FlexValue*> {
		if (data.type() != HspType::Struct) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE((FlexValue*)data.ptr()));
	}
}
