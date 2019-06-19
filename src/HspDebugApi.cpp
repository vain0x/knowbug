#include "hpiutil/hpiutil.hpp"
#include "HspDebugApi.h"

#undef max

HspDebugApi::HspDebugApi(HSP3DEBUG* debug)
	: debug_(debug)
	, context_(debug->hspctx)
	, exinfo_(debug->hspctx->exinfo2)
{
}

auto HspDebugApi::static_vars() -> PVal* {
	return context()->mem_var;
}

auto HspDebugApi::static_var_count() -> std::size_t {
	return context()->hsphed->max_val;
}

auto HspDebugApi::static_var_find_by_id(std::size_t static_var_id) -> std::optional<PVal*> {
	if (static_var_id >= static_var_count()) {
		return std::nullopt;
	}

	return std::make_optional(static_vars() + static_var_id);
}

auto HspDebugApi::static_var_find_by_name(char const* var_name) -> std::optional<std::size_t> {
	auto index = exinfo()->HspFunc_seekvar(var_name);

	if (index < 0 || (std::size_t)index >= static_var_count()) {
		return std::nullopt;
	}

	return std::make_optional((std::size_t)index);
}

auto HspDebugApi::static_var_find_name(std::size_t static_var_id) -> std::optional<std::string> {
	if (static_var_id >= static_var_count()) {
		return std::nullopt;
	}

	auto var_name = exinfo()->HspFunc_varname((int)static_var_id);
	if (!var_name) {
		return std::nullopt;
	}

	return std::make_optional<std::string>(var_name);
}
