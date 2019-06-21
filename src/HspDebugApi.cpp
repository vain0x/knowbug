#include "hpiutil/hpiutil.hpp"
#include "HspDebugApi.h"

#undef max

static auto pval_to_type(PVal const* pval) -> HspType {
	return (HspType)pval->flag;
}

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

auto HspDebugApi::static_var_find_by_name(char const* var_name) -> std::optional<std::size_t> {
	assert(var_name != nullptr);

	auto index = exinfo()->HspFunc_seekvar(var_name);

	if (index < 0 || (std::size_t)index >= static_var_count()) {
		return std::nullopt;
	}

	return std::make_optional((std::size_t)index);
}

auto HspDebugApi::static_var_find_name(std::size_t static_var_id) -> std::optional<std::string> {
	if (static_var_id >= static_var_count()) {
		throw new std::exception{ "Invalid static var id" };
	}

	auto var_name = exinfo()->HspFunc_varname((int)static_var_id);
	if (!var_name) {
		return std::nullopt;
	}

	return std::make_optional<std::string>(var_name);
}

auto HspDebugApi::static_var_to_pval(std::size_t static_var_id) -> PVal* {
	if (static_var_id >= static_var_count()) {
		throw new std::exception{ "invalid static var id" };
	}
	return static_vars() + static_var_id;
}

auto HspDebugApi::static_var_to_type(std::size_t static_var_id) -> HspType {
	return pval_to_type(static_var_to_pval(static_var_id));
}

auto HspDebugApi::static_var_to_data_ptr(std::size_t static_var_id) -> PDAT* {
	return hpiutil::PVal_getPtr(static_var_to_pval(static_var_id));
}

auto HspDebugApi::data_ptr_to_int(PDAT* data_ptr) -> HspInt {
	return *(HspInt const*)data_ptr;
}
