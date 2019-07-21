//! HSP の静的変数

#include "pch.h"
#include <array>
#include <memory>
#include <vector>
#include "../hpiutil/hpiutil.hpp"
#include "encoding.h"
#include "HspDebugApi.h"
#include "HspStaticVars.h"
#include "string_split.h"

static void fetch_static_var_names(HSP3DEBUG* debug_, std::size_t static_var_count, std::vector<Utf8String>& names) {
	names.reserve(static_var_count);

	auto p =
		std::unique_ptr<char, void(*)(char*)>
		{ debug_->get_varinf(nullptr, 0xFF)
		, debug_->dbg_close
		};

	for (auto&& var_name : StringLines{ std::string_view{ p.get() } }) {
		if (var_name.empty()) {
			continue;
		}

		names.emplace_back(to_utf8(as_hsp(var_name)));
	}
}

static auto seekSttVar(char const* name, HSPCTX* ctx, HSPEXINFO* exinfo) -> PVal*
{
	auto const index = int { exinfo->HspFunc_seekvar(name) };
	return (index >= 0) ? &ctx->mem_var[index] : nullptr;
}

HspStaticVars::HspStaticVars(HspDebugApi& api)
	: api_(api)
	, all_names_()
{
	fetch_static_var_names(api_.debug(), api_.static_var_count(), all_names_);
}

auto HspStaticVars::access_by_name(Utf8StringView const& var_name) -> PVal* {
	return seekSttVar(as_native(to_hsp(var_name)).data(), api_.context(), api_.exinfo());
}

auto HspStaticVars::find_name_by_pval(PVal* pval) -> std::optional<Utf8String> {
	auto begin = api_.static_vars();
	auto end = api_.static_vars() + api_.static_var_count();

	if (!(begin <= pval && pval <= end)) {
		return std::nullopt;
	}

	auto id = (std::size_t)(pval - begin);
	auto var_name = api_.static_var_find_name(id);
	if (!var_name) {
		return std::nullopt;
	}

	return std::make_optional(to_utf8(as_hsp(var_name->data())));
}
