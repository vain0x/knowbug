#pragma once

#include <array>
#include <cstdint>
#include "hpiutil/hpiutil_fwd.hpp"

using HspInt = std::int32_t;

using HspIndexes = std::array<std::size_t, hpiutil::ArrayDimMax>;

// HSP の変数が持つデータの型
enum class HspType
	: short
{
	Label = HSPVAR_FLAG_LABEL,
	Str = HSPVAR_FLAG_STR,
	Double = HSPVAR_FLAG_DOUBLE,
	Int = HSPVAR_FLAG_INT,
	Struct = HSPVAR_FLAG_STRUCT,
	Comstruct = HSPVAR_FLAG_COMSTRUCT,
};

// HSP の変数が持つデータへのポインタ
class HspData {
	HspType type_;
	PDAT* ptr_;

public:
	HspData(HspType type, PDAT* ptr)
		: type_(type)
		, ptr_(ptr)
	{
		if (ptr == nullptr) {
			throw new std::exception{ "Can't be null." };
		}
	}

	auto type() const -> HspType {
		return type_;
	}

	auto ptr() const -> PDAT* {
		return ptr_;
	}
};

class HspVarMetadata {
public:
	HspIndexes lengths_;
	std::size_t element_size_;
	std::size_t data_size_;
	std::size_t block_size_;
	void const* data_ptr_;
	void const* master_ptr_;
	void const* block_ptr_;

public:
	auto lengths() const -> HspIndexes const& {
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
};
