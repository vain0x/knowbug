#pragma once

#include "hsx_types_fwd.h"

namespace hsp_sdk_ext {
	// HSP の変数が持つデータへのポインタ
	// FIXME: size を持たせる？
	// FIXME: HspValue に改名
	// FIXME: const?
	class HspData {
		HspType type_;

		// FIXME: data_ に改名
		PDAT* ptr_;

	public:
		HspData()
			: type_(HspType::None)
			, ptr_()
		{
		}

		HspData(HspType type, PDAT* ptr)
			: type_(type)
			, ptr_(ptr)
		{
			assert(type == HspType::None || ptr != nullptr);
		}

		auto type() const -> HspType {
			return type_;
		}

		auto ptr() const -> PDAT* {
			return ptr_;
		}
	};
}
