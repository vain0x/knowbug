#pragma once

#include "hsx_types_fwd.h"

namespace hsx {
	// HSP の引数 (パラメータスタックの1要素) への参照。
	// FIXME: size を持たせる
	class HspParamData {
		STRUCTPRM const* param_;

		std::size_t param_index_;

		void const* ptr_;

		bool safety_;

	public:
		HspParamData(STRUCTPRM const* param, std::size_t param_index, void const* ptr, bool safety)
			: param_(param)
			, param_index_(param_index)
			, ptr_(ptr)
			, safety_(safety)
		{
			assert(param != nullptr);
			assert(ptr != nullptr);
		}

		auto param() const -> STRUCTPRM const* {
			return param_;
		}

		auto param_index() const -> std::size_t {
			return param_index_;
		}

		auto ptr() const -> void const* {
			return ptr_;
		}

		bool safety() const {
			return safety_;
		}
	};
}
