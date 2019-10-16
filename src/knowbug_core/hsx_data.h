#pragma once

#include "hsx_types_fwd.h"

namespace hsx {
	// HSP の変数が持つデータへのポインタ
	// FIXME: size を持たせる？
	class HspData {
		HspType type_;

		// label, double, int, struct: 変数が持つバッファの一部へのポインタ
		// str: 文字列自身へのポインタ
		PDAT const* ptr_;

	public:
		HspData(HspType type, PDAT const* ptr)
			: type_(type)
			, ptr_(ptr)
		{
			assert(type == HspType::None || ptr != nullptr);
		}

		auto type() const -> HspType {
			return type_;
		}

		auto ptr() const -> PDAT const* {
			return ptr_;
		}
	};
}
