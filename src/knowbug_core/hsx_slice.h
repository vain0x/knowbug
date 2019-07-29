#pragma once

#include <optional>
#include "hsx_types_fwd.h"

namespace hsp_sdk_ext {
	// 配列や vector のある範囲に対する読み取り専用の参照
	template<typename T>
	class Slice {
		T const* data_;

		std::size_t size_;

	public:
		Slice()
			: data_()
			, size_()
		{
		}

		Slice(T const* data, std::size_t size)
			: data_(data)
			, size_(data != nullptr ? size : 0)
		{
		}

		auto data() const -> T const* {
			return data_;
		}

		auto size() const -> std::size_t {
			return size_;
		}

		auto begin() const -> T const* {
			return data();
		}

		auto end() const -> T const* {
			return data() + size();
		}

		auto get(std::size_t index) const -> std::optional<T const*> {
			if (index >= size()) {
				return std::nullopt;
			}

			return std::make_optional(begin() + index);
		}

		auto get_unchecked(std::size_t index) const -> T const* {
			return begin() + index;
		}

		auto slice(std::size_t offset, std::size_t count) const->Slice<T> {
			offset = std::min(offset, size());
			count = std::min(count, size() - offset);
			return Slice<T>{ begin() + offset, count };
		}
	};
}
