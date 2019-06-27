#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include "hpiutil/hpiutil_fwd.hpp"

// code segment の先頭からのオフセット。ラベルが指す位置を表現する。
using HspCodeOffset = std::int32_t;

// code segment 内へのポインタ (有効) または nullptr (無効)
// このラベルの位置にラベルが書かれているとは限らない (newlab 命令で生成された可能性がある)。
using HspLabel = unsigned short const*;

using HspStr = char*;

using HspDouble = double;

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
			assert(false && u8"HspData can't be null.");
			throw new std::invalid_argument{ u8"ptr" };
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

	bool safety_;

public:
	HspParamStack(STRUCTDAT const* struct_dat, void* ptr, bool safety)
		: struct_dat_(struct_dat)
		, ptr_(ptr)
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

	// 引数のデータの読み取りが安全か
	bool safety() const {
		return safety_;
	}
};

// HSP の引数 (引数リストの1要素) への参照
class HspParamData {
	STRUCTPRM const* param_;
	std::size_t param_index_;
	void* ptr_;
	bool safety_;

public:
	HspParamData(STRUCTPRM const* param, std::size_t param_index, void* ptr, bool safety)
		: param_(param)
		, param_index_(param_index)
		, ptr_(ptr)
		, safety_(safety)
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

	bool safety() const {
		return safety_;
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

// システム変数の種類。
// 順番は名前順、ただし類似したものは近くに集める。
enum HspSystemVarKind {
	Cnt = 1,
	Err,
	IParam,
	WParam,
	LParam,
	LoopLev,
	SubLev,
	Refstr,
	Refdval,
	Stat,
	StrSize,
};
