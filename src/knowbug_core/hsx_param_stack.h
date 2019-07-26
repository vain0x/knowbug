#pragma once

#include "hsx_types_fwd.h"

namespace hsp_sdk_ext {
	// HSP のパラメータスタックへの参照。
	// パラメータスタックは、コマンドの実引数、またはインスタンスのメンバ変数の実データが格納される領域。
	class HspParamStack {
		STRUCTDAT const* struct_dat_;

		// FIXME: data_ に改名
		void* ptr_;

		std::size_t size_;

		// FIXME: safe_ に改名
		bool safety_;

	public:
		HspParamStack(STRUCTDAT const* struct_dat, void* ptr, std::size_t size, bool safety)
			: struct_dat_(struct_dat)
			, ptr_(ptr)
			, size_(size)
			, safety_(safety)
		{
			assert(struct_dat != nullptr);
			assert(ptr != nullptr);
		}

		auto struct_dat() const -> STRUCTDAT const* {
			return struct_dat_;
		}

		auto ptr() const -> void* {
			return ptr_;
		}

		auto size() const -> std::size_t {
			return size_;
		}

		// 引数のデータの読み取りが安全か
		bool safety() const {
			return safety_;
		}
	};
}
