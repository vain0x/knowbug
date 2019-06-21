#pragma once

#include <cstdint>
#include "hpiutil/hpiutil_fwd.hpp"

using HspInt = std::int32_t;

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
