#pragma once

#include "hsx_types_fwd.h"

namespace hsx {
	// HSP の変数が持つデータへのポインタ
	// FIXME: size を持たせる？
	class HspData {
		HsxVartype type_;

		// label, double, int, struct: 変数が持つバッファの一部へのポインタ
		// str: 文字列自身へのポインタ
		PDAT const* ptr_;

	public:
		HspData(HsxVartype type, PDAT const* ptr)
			: type_(type)
			, ptr_(ptr)
		{
			assert(type == HSPVAR_FLAG_NONE || ptr != nullptr);
		}

		auto type() const -> HsxVartype {
			return type_;
		}

		auto ptr() const -> PDAT const* {
			return ptr_;
		}
	};
}
