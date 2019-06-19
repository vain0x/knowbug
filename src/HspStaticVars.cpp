//! HSP の静的変数

#include "hpiutil/hpiutil.hpp"
#include "HspStaticVars.h"

static auto seekSttVar(char const* name, HSPEXINFO* exinfo) -> PVal*
{
	auto const index = int { exinfo->HspFunc_seekvar(name) };
	return (index >= 0) ? &ctx->mem_var[index] : nullptr;
}

HspStaticVars::HspStaticVars(HSPEXINFO* exinfo)
	: exinfo_(exinfo)
{
}

auto HspStaticVars::access_by_name(char const* var_name) -> PVal* {
	return seekSttVar(var_name, exinfo_);
}
