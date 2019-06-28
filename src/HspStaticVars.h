//! HSP の静的変数

#pragma once

#include <optional>
#include "hpiutil/hpiutil_fwd.hpp"
#include "encoding.h"

class HspDebugApi;

// FIXME: おそらくメンバ変数は HspObjects に持たせて、これを構築する関数を公開するのがよい。
class HspStaticVars {
	HspDebugApi& api_;

	std::vector<HspString> all_names_;

public:
	HspStaticVars(HspDebugApi& api);

	auto find_id(char const* var_name) -> std::optional<std::size_t>;

	auto get_all_names() const -> std::vector<HspString> const& {
		return all_names_;
	}

	auto access_by_name(char const* var_name) -> PVal*;

	auto find_name_by_pval(PVal* pval) -> std::optional<HspString>;
};
