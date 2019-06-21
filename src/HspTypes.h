#pragma once

#include <cstdint>
#include "hpiutil/hpiutil_fwd.hpp"

using HspInt = std::int32_t;

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
