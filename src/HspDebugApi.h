#pragma once

#include <optional>
#include <string>
#include "hpiutil/hpiutil_fwd.hpp"
#include "HspTypes.h"

// HSP SDK のラッパー
// 配列アクセスの範囲検査、null 検査、整数と enum の変換など、小さい仕事をする。
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

	auto static_var_find_by_name(char const* var_name) -> std::optional<std::size_t>;

	auto static_var_find_name(std::size_t static_var_id) -> std::optional<std::string>;

	auto static_var_to_pval(std::size_t static_var_id) -> PVal*;

	auto static_var_to_type(std::size_t static_var_id) -> HspType;

	auto static_var_to_data_ptr(std::size_t static_var_id) -> PDAT*;

	auto data_ptr_to_int(PDAT* data_ptr) -> HspInt;
};
