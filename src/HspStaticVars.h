//! HSP の静的変数

#pragma once

#include "hpiutil/hpiutil_fwd.hpp"

class HspStaticVars {
	HSPEXINFO* exinfo_;

public:
	HspStaticVars(HSPEXINFO* exinfo);

	auto access_by_name(char const* var_name) -> PVal*;
};
