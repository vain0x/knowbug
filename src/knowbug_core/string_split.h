//! 文字列の分割

#pragma once

#include <cassert>
#include <string>
#include <vector>
#include "test_suite.h"

extern void string_lines_tests(Tests& tests);

// 文字列を行ごとに分割するイテレータ
template<typename TChar>
class StringLineIterator {
	using Self = StringLineIterator<TChar>;
	using Str = std::basic_string_view<TChar>;

	Str str_;

	// 次の行の開始位置。`str_.size() + 1` (終端文字の後ろ) で停止。
	std::size_t head_;

	// 最後に取得した行。
	std::optional<Str> last_;

	// イテレータが停止したら true。
	bool end_;

public:
	StringLineIterator(Str str)
		: str_(std::move(str))
		, head_()
		, last_()
		, end_()
	{
		assert(head_ <= str_.size() + 1);
	}

	auto operator ==(Self const& other) const -> bool {
		// 「停止したイテレータ」同士は等しいとみなす。
		if (end_ || other.end_) {
			return end_ == other.end_;
		}

		return str_ == other.str_
			&& head_ == other.head_;
	}

	auto operator !=(Self const& other) const -> bool {
		return !(*this == other);
	}

	// イテレータを停止する。
	auto stop() -> Self& {
		end_ = true;
		last_ = std::nullopt;
		return *this;
	}

	// イテレータを前進させる。
	auto operator ++() -> Self& {
		last_ = next();
		if (!last_) {
			end_ = true;
		}
		return *this;
	}

	// イテレータがいま指している値を取得する。
	auto operator *() -> Str {
		return *last_;
	}

	// イテレータから次の値を取り出す。
	auto next() -> std::optional<Str> {
		if (end_ || head_ > str_.size()) {
			return std::nullopt;
		}

		auto l = head_;
		auto r = advance();

		assert(l <= r);
		auto count = r - l;

		return std::make_optional<Str>(str_.data() + l, count);
	}

private:
	// 次の行の終端位置を返し、head を進める。
	auto advance() -> std::size_t {
		assert(head_ <= str_.size() + 1);

		auto i = head_;

		// EOF か LF を探す。
		while (i < str_.size() && str_[i] != TChar{ '\n' }) {
			i++;
		}

		// 行末の位置を調整する。
		auto r = i;
		if (r >= 1 && str_[r - 1] == TChar{ '\r' }) {
			r--;
		}

		// LF を飛ばす。
		head_ = i + 1;
		return r;
	}
};

template<typename TChar>
class StringLines {
	using Iter = StringLineIterator<TChar>;

	std::basic_string_view<TChar> str_;

public:
	StringLines(std::basic_string_view<TChar> str)
		: str_(std::move(str))
	{
	}

	auto iter() const -> Iter {
		return Iter{ str_ };
	}

	auto begin() const -> Iter {
		return ++iter();
	}

	auto end() const -> Iter {
		return Iter{ str_ }.stop();
	}

	auto to_vector() const -> std::vector<std::basic_string_view<TChar>> {
		auto lines = std::vector<std::basic_string_view<TChar>>{};

		for (auto&& line : *this) {
			lines.emplace_back(std::move(line));
		}

		return lines;
	}
};
