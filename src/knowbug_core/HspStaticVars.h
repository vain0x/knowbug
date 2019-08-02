//! HSP の静的変数

#pragma once

#include <optional>
#include "encoding.h"

class HspDebugApi;

// FIXME: おそらくメンバ変数は HspObjects に持たせて、これを構築する関数を公開するのがよい。
class HspStaticVars {
	HspDebugApi& api_;

	std::vector<Utf8String> all_names_;

public:
	HspStaticVars(HspDebugApi& api);

	auto get_all_names() const -> std::vector<Utf8String> const& {
		return all_names_;
	}
};
