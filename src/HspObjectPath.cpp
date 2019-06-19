#include "HspObjects.h"
#include "HspObjectPath.h"

auto HspObjectPath::name(HspObjects& objects) const -> std::string {
	return objects.static_var_name(static_var_id());
}

bool HspObjectPath::is_array(HspObjects& objects) const {
	return objects.static_var_is_array(static_var_id());
}
