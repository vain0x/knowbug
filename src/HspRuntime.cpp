#include "HspRuntime.h"

HspRuntime::HspRuntime(HSPCTX* ctx, HSP3DEBUG* debug)
	: ctx_(ctx)
	, debug_(debug)
	, static_vars_(exinfo())
{
}
