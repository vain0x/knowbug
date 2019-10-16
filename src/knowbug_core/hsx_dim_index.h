#pragma once

#include <array>
#include "hsx_types_fwd.h"

namespace hsx {
	// 多次元配列へのインデックス。(最大4次元)
	// 多次元配列の要素の位置を表す。あるいは、次元数と要素数を表す。
	class HspDimIndex {
	public:
		static constexpr auto MAX_DIM = std::size_t{ 4 };

	private:
		// 次元数 (1-4)
		std::size_t dim_;

		std::array<std::size_t, MAX_DIM> indexes_;

	public:
		HspDimIndex()
			: dim_(1)
			, indexes_({ 0, 0, 0, 0 })
		{
		}

		HspDimIndex(std::size_t dim, std::array<std::size_t, MAX_DIM> const& indexes)
			: dim_(dim)
			, indexes_(indexes)
		{
			if (!(1 <= dim && dim <= MAX_DIM)) {
				assert(false && u8"dim must be 1-4");
				throw new std::exception{};
			}
		}

		static auto one() -> HspDimIndex {
			return HspDimIndex{ 1, { 1, 0, 0, 0 } };
		}

		bool operator ==(HspDimIndex const& other) const {
			return dim() == other.dim() && indexes_ == other.indexes_;
		}

		// 次元数
		auto dim() const -> std::size_t {
			return dim_;
		}

		auto operator[](std::size_t i) const -> std::size_t {
			return at(i);
		}

		auto at(std::size_t i) const -> std::size_t {
			if (i >= dim()) {
				assert(false && u8"次元数を超えています");
				throw std::exception{};
			}

			return indexes_[i];
		}

		auto begin() const -> std::size_t const* {
			return indexes_.data();
		}

		auto end() const -> std::size_t const* {
			assert(dim() <= indexes_.size());
			return indexes_.data() + dim();
		}

		// 要素数
		auto size() const -> std::size_t {
			auto count = std::size_t{ 1 };
			for (auto i : *this) {
				count *= i;
			}
			return count;
		}
	};
}
