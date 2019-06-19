#include "HspDebugApi.h"
#include "HspRuntime.h"

HspRuntime::HspRuntime(HspDebugApi&& api)
	: api_(std::move(api))
	, static_vars_(api_)
{
}
