#include "pch.h"
#include "hsx_data.h"
#include "hsx_internals.h"

namespace hsp_sdk_ext {
	auto data_from_label(HspLabel* ptr) -> HspData {
		return HspData{ HspType::Label, (PDAT*)ptr };
	}

	auto data_from_str(HspStr value) -> HspData {
		return HspData{ HspType::Str, (PDAT*)value };
	}

	auto data_from_double(HspDouble* ptr) -> HspData {
		return HspData{ HspType::Double, (PDAT*)ptr };
	}

	auto data_from_int(HspInt* ptr) -> HspData {
		return HspData{ HspType::Int, (PDAT*)ptr };
	}
}
