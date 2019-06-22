#pragma once

#include <array>
#include <cstdint>
#include "hpiutil/hpiutil_fwd.hpp"

using HspStr = char*;

using HspInt = std::int32_t;

using HspIndexes = std::array<std::size_t, hpiutil::ArrayDimMax>;

// 引数の種類
// MPTYPE_*
using HspParamType = short;

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

// HSP の引数リストへの参照
class HspParamStack {
	STRUCTDAT const* struct_dat_;
	void* ptr_;

public:
	HspParamStack(STRUCTDAT const* struct_dat, void* ptr)
		: struct_dat_(struct_dat)
		, ptr_(ptr)
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
};

// HSP の引数 (引数リストの1要素) への参照
class HspParamData {
	STRUCTPRM const* param_;
	std::size_t param_index_;
	void* ptr_;

public:
	HspParamData(STRUCTPRM const* param, std::size_t param_index, void* ptr)
		: param_(param)
		, param_index_(param_index)
		, ptr_(ptr)
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

	auto ptr() const -> void* {
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
