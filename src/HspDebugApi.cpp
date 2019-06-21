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

auto HspDebugApi::var_to_type(PVal* pval) -> HspType {
	return pval_to_type(pval);
}

auto HspDebugApi::var_to_data(PVal* pval) -> HspData {
	auto type = pval_to_type(pval);
	auto pdat = hpiutil::PVal_getPtr(pval);
	return HspData{ type, pdat };
}

auto HspDebugApi::var_element_count(PVal* pval) -> std::size_t {
	return hpiutil::PVal_cntElems((PVal*)pval);
}

auto HspDebugApi::var_element_to_indexes(PVal* pval, std::size_t aptr) -> HspIndexes {
	// FIXME: 2次元以上のケースを実装
	auto indexes = HspIndexes{};
	indexes[0] = aptr;
	return indexes;
}

auto HspDebugApi::var_element_to_aptr(PVal* pval, HspIndexes const& indexes) -> std::size_t {
	// FIXME: 2次元以上のケースを実装
	return indexes[0];
}

auto HspDebugApi::var_element_to_data(PVal* pval, std::size_t aptr) -> HspData {
	auto type = pval_to_type(pval);
	auto ptr = hpiutil::PVal_getPtr(pval, aptr);
	return HspData{ type, ptr };
}

auto HspDebugApi::data_to_int(HspData const& data) const -> HspInt {
	if (data.type() != HspType::Int) {
		throw new std::exception{ "Invalid type" };
	}
	return *(HspInt const*)data.ptr();
}
