//! HSP の静的変数

#include <array>
#include <memory>
#include <vector>
#include "hpiutil/hpiutil.hpp"
#include "module/supio/supio.h"
#include "encoding.h"
#include "HspDebugApi.h"
#include "HspStaticVars.h"

static void fetch_static_var_names(HSP3DEBUG* debug_, std::size_t static_var_count, std::vector<HspString>& names) {
	names.reserve(static_var_count);

	auto p =
		std::unique_ptr<char, void(*)(char*)>
		{ debug_->get_varinf(nullptr, 0xFF)
		, debug_->dbg_close
		};
	strsp_ini();
	for ( ;; ) {
		auto name = std::array<char, 0x100> {};
		auto const chk = strsp_get(p.get(), name.data(), '\0', name.size() - 1);
		if ( chk == 0 ) break;
		names.emplace_back(as_hsp(name.data()));
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

auto HspStaticVars::access_by_name(char const* var_name) -> PVal* {
	return seekSttVar(var_name, api_.context(), api_.exinfo());
}

auto HspStaticVars::find_id(char const* var_name) -> std::optional<std::size_t> {
	return api_.static_var_find_by_name(var_name);
}

auto HspStaticVars::find_name_by_pval(PVal* pval) -> std::optional<HspString> {
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

	return std::make_optional(to_owned(as_hsp(var_name->data())));
}
