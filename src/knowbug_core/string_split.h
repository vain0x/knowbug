//! 文字列の分割

#pragma once

#include <cassert>
#include <string>
#include <vector>
#include "test_suite.h"

extern void string_to_lines_tests(Tests& tests);

// NULL 文字によって分割された文字列
template<typename TChar>
class SplitString {
	std::basic_string<TChar> buffer_;

	// NULL 文字によって分割された文字列の各部分の範囲
	std::vector<std::pair<std::size_t, std::size_t>> ranges_;

public:
	SplitString(std::basic_string<TChar>&& buffer, std::vector<std::pair<std::size_t, std::size_t>>&& ranges)
		: buffer_(std::move(buffer))
		, ranges_(ranges)
	{
	}

	// 分割された部分の個数
	auto size() const -> std::size_t {
		return ranges_.size();
	}

	auto at(std::size_t index) const -> std::basic_string_view<TChar> {
		auto range = ranges_.at(index);

		assert(range.first <= range.second);
		auto count = range.second - range.first;

		return std::basic_string_view<TChar>{ buffer_.data() + range.first, count };
	}
};

// 文字列を行ごとに分割する。
template<typename TChar>
auto string_to_lines(std::basic_string<TChar>&& str) -> SplitString<TChar> {
	auto ranges = std::vector<std::pair<std::size_t, std::size_t>>{};

	// いま見ている位置
	auto i = std::size_t{};

	// いま見ている位置が含まれる行の開始位置
	auto head = std::size_t{};

	while (i <= str.size()) {
		// EOF または行末
		if (i == str.size() || str[i] == TChar{ '\n' }) {
			// 行末の位置
			auto r = i;
			if (r >= 1 && str[r - 1] == TChar{ '\r' }) {
				r--;
			}

			// 行頭の位置 (字下げを除く)
			auto l = head;

			// l..r が1行の範囲
			ranges.emplace_back(l, r);

			// LF を飛ばす。
			i++;
			head = i;
			continue;
		}

		i++;
	}

	return SplitString{ std::move(str), std::move(ranges) };
}
