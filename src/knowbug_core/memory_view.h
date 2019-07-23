#pragma once

#include <cassert>
#include <cstddef>

class MemoryView {
	void const* data_;

	std::size_t size_;

public:
	MemoryView()
		: data_()
		, size_()
	{
	}

	MemoryView(void const* data, std::size_t size)
		: data_(data)
		, size_(size)
	{
		assert(data != nullptr || size == 0);
	}

	auto data() const -> void const* {
		return data_;
	}

	auto size() const -> std::size_t {
		return size_;
	}
};
