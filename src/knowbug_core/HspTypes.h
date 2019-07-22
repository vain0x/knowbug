#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include "../hpiutil/hpiutil_fwd.hpp"

#undef min

// code segment の先頭からのオフセット。ラベルが指す位置を表現する。
using HspCodeOffset = std::int32_t;

// code segment 内へのポインタ (有効) または nullptr (無効)
// このラベルの位置にラベルが書かれているとは限らない (newlab 命令で生成された可能性がある)。
using HspLabel = unsigned short const*;

using HspStr = char*;

using HspDouble = double;

using HspInt = std::int32_t;

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

enum class HspVarMode
	: short
{
	None = HSPVAR_MODE_NONE,
	Alloc = HSPVAR_MODE_MALLOC,
	Clone = HSPVAR_MODE_CLONE,
};

// 多次元配列へのインデックス。(最大4次元)
// 多次元配列の要素の位置を表す。あるいは、次元数と要素数を表す。
class HspDimIndex {
public:
	static std::size_t const MAX_DIM = 4;

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

	HspDimIndex(std::size_t dim, std::array<std::size_t, MAX_DIM> indexes)
		: dim_(dim)
		, indexes_(indexes)
	{
		assert(1 <= dim && dim <= MAX_DIM);
	}

	bool operator ==(HspDimIndex const& other) const {
		return dim() == other.dim() && indexes_ == other.indexes_;
	}

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
		return indexes_.data() + std::min(MAX_DIM, dim());
	}

	auto size() const -> std::size_t {
		auto count = std::size_t{ 1 };
		for (auto i : *this) {
			count *= i;
		}
		return count;
	}
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
	HspType type_;
	HspVarMode mode_;
	HspDimIndex lengths_;
	std::size_t element_size_;
	std::size_t data_size_;
	std::size_t block_size_;
	void const* data_ptr_;
	void const* master_ptr_;
	void const* block_ptr_;

public:
	auto type() const -> HspType {
		return type_;
	}

	auto mode() const -> HspVarMode {
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
};

// システム変数の種類。
// 順番は名前順、ただし類似したものは近くに集める。
// FIXME: thismod を追加
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
