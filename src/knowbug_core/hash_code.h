// ハッシュコード計算用のヘルパー

#pragma once

#include <cstddef>

// ハッシュテーブル用のハッシュコード
// 参考: <https://www.cpp.edu/~ftang/courses/CS240/lectures/hashing.htm>
class HashCode {
	std::size_t value_;

public:
	HashCode()
		: value_()
	{
	}

	explicit HashCode(std::size_t value)
		: value_(value)
	{
	}

	template<typename T>
	static auto from(T const& value) -> HashCode {
		return HashCode{ std::hash<T>{}(value) };
	}

	template<typename T>
	auto combine(T const& other) const -> HashCode {
		auto k = std::hash<T>{}(other);
		return HashCode{ (value_ << 5 | value_ >> 27) ^ k };
	}

	auto value() const -> std::size_t {
		return value_;
	}
};
