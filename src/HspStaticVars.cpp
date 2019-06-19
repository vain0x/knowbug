//! HSP の静的変数

#include <array>
#include <memory>
#include <vector>
#include "hpiutil/hpiutil.hpp"
#include "module/supio/supio.h"
#include "HspStaticVars.h"

static void fetch_static_var_names(HSP3DEBUG* debug_, std::vector<HspString>& names) {
	names.reserve(hpiutil::staticVars().size());

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
		names.emplace_back(name.data());
	}
}

static auto seekSttVar(char const* name, HSPEXINFO* exinfo) -> PVal*
{
	auto const index = int { exinfo->HspFunc_seekvar(name) };
	return (index >= 0) ? &ctx->mem_var[index] : nullptr;
}

HspStaticVars::HspStaticVars(HSP3DEBUG* debug, HSPEXINFO* exinfo)
	: debug_(debug)
	, exinfo_(exinfo)
	, all_names_()
{
	fetch_static_var_names(debug, all_names_);
}

auto HspStaticVars::access_by_name(char const* var_name) -> PVal* {
	return seekSttVar(var_name, exinfo_);
}
