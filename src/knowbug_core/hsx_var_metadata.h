#pragma once

#include "hsx_types_fwd.h"
#include "hsx_dim_index.h"

namespace hsx {
	// HSP の変数に関連するメタデータの詰め合わせ
	class HspVarMetadata {
	public:
		HsxVartype type_;
		HsxVarMode mode_;
		HspDimIndex lengths_;
		std::size_t element_size_;
		std::size_t data_size_;
		std::size_t block_size_;
		void const* data_ptr_;
		void const* master_ptr_;
		void const* block_ptr_;

	public:
		auto type() const -> HsxVartype {
			return type_;
		}

		auto mode() const -> HsxVarMode {
			return mode_;
		}

		auto lengths() const -> HspDimIndex const& {
			return lengths_;
		}

		auto element_size() const -> std::size_t {
			return element_size_;
		}

		auto data_size() const -> std::size_t {
			return data_size_;
		}

		auto block_size() const -> std::size_t {
			return block_size_;
		}

		auto data_ptr() const -> void const* {
			return data_ptr_;
		}

		auto master_ptr() const -> void const* {
			return master_ptr_;
		}

		auto block_ptr() const -> void const* {
			return block_ptr_;
		}

		static auto none() -> HspVarMetadata {
			return HspVarMetadata{
				HSPVAR_FLAG_NONE,
				HSPVAR_MODE_NONE,
				HspDimIndex::one(),
				0,
				0,
				0,
				nullptr,
				nullptr,
				nullptr,
			};
		}
	};
}
