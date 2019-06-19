//! HSP SDK の薄いラッパー

#pragma once

#include <optional>
#include <string>
#include "hpiutil/hpiutil_fwd.hpp"

class HspDebugApi {
	HSPCTX* context_;

	HSP3DEBUG* debug_;

	HSPEXINFO* exinfo_;

public:
	HspDebugApi(HSP3DEBUG* debug);

	auto context() -> HSPCTX* {
		return context_;
	}

	auto debug() -> HSP3DEBUG* {
		return debug_;
	}

	auto exinfo() -> HSPEXINFO* {
		return exinfo_;
	}

	auto static_vars() -> PVal*;

	auto static_var_count() -> std::size_t;

	auto static_var_find_by_id(std::size_t static_var_id) -> std::optional<PVal*>;

	auto static_var_find_by_name(char const* var_name) -> std::optional<std::size_t>;

	auto static_var_find_name(std::size_t static_var_id) -> std::optional<std::string>;
};
