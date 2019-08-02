//! HSP の静的変数

#include "pch.h"
#include <array>
#include <memory>
#include <vector>
#include "encoding.h"
#include "HspDebugApi.h"
#include "HspStaticVars.h"
#include "string_split.h"

namespace hsx = hsp_sdk_ext;

static void fetch_static_var_names(HSP3DEBUG* debug_, std::size_t static_var_count, std::vector<Utf8String>& names) {
	names.reserve(static_var_count);

	auto p = hsx::debug_to_static_var_names(debug_);

	for (auto&& var_name : StringLines{ std::string_view{ p.get() } }) {
		if (var_name.empty()) {
			continue;
		}

		names.emplace_back(to_utf8(as_hsp(var_name)));
	}
}

HspStaticVars::HspStaticVars(HspDebugApi& api)
	: api_(api)
	, all_names_()
{
	fetch_static_var_names(api_.debug(), api_.static_var_count(), all_names_);
}
