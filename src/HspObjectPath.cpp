#include "HspObjects.h"
#include "HspObjectPath.h"

HspObjectPath::~HspObjectPath() {
}

HspObjectPath::StaticVar::StaticVar(std::shared_ptr<HspObjectPath> parent, std::size_t static_var_id)
	: parent_(parent)
	, static_var_id_(static_var_id)
{
}

auto HspObjectPath::StaticVar::name(HspObjects& objects) const -> std::string {
	return objects.static_var_name(static_var_id());
}

bool HspObjectPath::StaticVar::is_array(HspObjects& objects) const {
	return objects.static_var_is_array(static_var_id());
}
