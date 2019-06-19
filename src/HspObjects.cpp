#include "hpiutil/hpiutil.hpp"
#include "HspDebugApi.h"
#include "HspObjects.h"
#include "HspStaticVars.h"

auto HspObjects::static_var_name(std::size_t static_var_id)->std::string {
	return *api_.static_var_find_name(static_var_id);
}

bool HspObjects::static_var_is_array(std::size_t static_var_id) {
	return hpiutil::PVal_isStandardArray(&hpiutil::staticVars()[static_var_id]);
}

auto HspObjects::static_var_to_pval(std::size_t static_var_id)->PVal* {
	return (PVal*)&hpiutil::staticVars()[static_var_id];
}
