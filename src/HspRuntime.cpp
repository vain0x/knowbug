#include "hpiutil/dinfo.hpp"
#include "HspDebugApi.h"
#include "HspRuntime.h"

HspRuntime::HspRuntime(HspDebugApi&& api)
	: api_(std::move(api))
	, static_vars_(api_)
	, objects_(api_, static_vars_, hpiutil::DInfo::instance())
{
}
