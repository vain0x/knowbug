//! HSP の静的変数

#pragma once

#include "hpiutil/hpiutil_fwd.hpp"
#include "encoding.h"

class HspStaticVars {
	HSP3DEBUG* debug_;
	HSPEXINFO* exinfo_;

	std::vector<HspString> all_names_;

public:
	HspStaticVars(HSP3DEBUG* debug, HSPEXINFO* exinfo);

	auto get_all_names() const -> std::vector<HspString> const& {
		return all_names_;
	}

	auto access_by_name(char const* var_name) -> PVal*;
};
